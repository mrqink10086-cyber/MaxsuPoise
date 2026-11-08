import argparse
import hashlib
import io
import json
import struct
from pathlib import Path

import capstone
import pefile


def address_library(path):
    data = io.BytesIO(path.read_bytes())

    def read(fmt):
        return struct.unpack('<' + fmt, data.read(struct.calcsize('<' + fmt)))[0]

    if read('I') not in (1, 2):
        raise ValueError('Unsupported Address Library format')
    version = [read('I') for _ in range(4)]
    data.read(read('I'))
    pointer_size, count = read('I'), read('I')
    result = {}
    previous_id = previous_offset = 0

    def value(kind, previous):
        if kind == 0:
            return read('Q')
        if kind == 1:
            return previous + 1
        if kind in (2, 3, 4, 5):
            delta = read('B' if kind < 4 else 'H')
            return previous + delta if kind % 2 == 0 else previous - delta
        return read('H' if kind == 6 else 'I')

    for _ in range(count):
        kind = read('B')
        current_id = value(kind & 15, previous_id)
        high = kind >> 4
        offset = value(high & 7, previous_offset // pointer_size if high & 8 else previous_offset)
        if high & 8:
            offset *= pointer_size
        result[current_id] = offset
        previous_id, previous_offset = current_id, offset
    return version, result


def audit(exe, library):
    version, ids = address_library(library)
    if version[:3] not in ([1, 5, 97], [1, 6, 1170]):
        raise ValueError(f'Unsupported runtime: {version}')
    ae = version[1] == 6
    pe = pefile.PE(str(exe), fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    checks = []

    for name, table_id, target_id in (
        ('Character::Update(float)', 207886 if ae else 261397, 37348 if ae else 36357),
        ('PlayerCharacter::Update(float)', 208040 if ae else 261916, 40447 if ae else 39375),
    ):
        slot = ids[table_id] + 0xAD * 8
        target = struct.unpack('<Q', pe.get_data(slot, 8))[0] - base
        checks.append(dict(name=name, kind='vtable', table_id=table_id,
                           table_rva=hex(ids[table_id]), slot='0xAD',
                           target_id=target_id, expected_rva=hex(ids[target_id]),
                           actual_rva=hex(target), passed=target == ids[target_id]))

    for name, caller_id, offset, target_id in (
        ('HitEventHandler', 38627 if ae else 37673, 0x4A8 if ae else 0x3C0, 38586 if ae else 37633),
        ('MagicStaggerHook', 34982 if ae else 34188, 0x6F, 37710 if ae else 36700),
        ('PerkEntryHook_Aggressor', 37710 if ae else 36700, 0xA1 if ae else 0x9A, 23526 if ae else 23073),
        ('PerkEntryHook_Target', 37710 if ae else 36700, 0xB9 if ae else 0xAE, 23526 if ae else 23073),
    ):
        site = ids[caller_id] + offset
        code = pe.get_data(site, 5)
        target = site + 5 + struct.unpack('<i', code[1:])[0]
        context = [f'{ins.address - base:#x}: {ins.mnemonic} {ins.op_str}'
                   for ins in md.disasm(pe.get_data(ids[caller_id], offset + 5), base + ids[caller_id])][-12:]
        checks.append(dict(name=name, kind='call', caller_id=caller_id,
                           caller_rva=hex(ids[caller_id]), offset=hex(offset), site_rva=hex(site),
                           bytes=code.hex(' '), target_id=target_id,
                           expected_rva=hex(ids[target_id]), actual_rva=hex(target),
                           context=context, passed=code[0] == 0xE8 and target == ids[target_id]))

    return dict(runtime='.'.join(map(str, version)), exe=str(exe.resolve()),
                exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),
                address_library=str(library.resolve()),
                address_library_sha256=hashlib.sha256(library.read_bytes()).hexdigest(),
                passed=all(item['passed'] for item in checks), checks=checks)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('exe', type=Path)
    parser.add_argument('library', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    report = audit(args.exe, args.library)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    for check in report['checks']:
        print(('PASS' if check['passed'] else 'FAIL') + ': ' + check['name'])
    raise SystemExit(0 if report['passed'] else 1)
