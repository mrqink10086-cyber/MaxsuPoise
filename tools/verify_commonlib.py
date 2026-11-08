import argparse
import base64
import hashlib
import json
import urllib.request
from datetime import datetime, timezone
from pathlib import Path


def get_json(url):
    request = urllib.request.Request(url, headers={'User-Agent': 'MaxsuPoise-upstream-check'})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


def git_hash(data):
    return hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    api = 'https://api.github.com/repos/alandtse/CommonLibSSE-NG'
    commit = get_json(api + '/commits/ng')
    sha = commit['sha']
    tree = get_json(api + '/git/trees/' + sha + '?recursive=1')
    if tree.get('truncated'):
        raise RuntimeError('GitHub returned a truncated tree')
    missing, changed, line_endings = [], [], []
    files = [entry for entry in tree['tree'] if entry['type'] == 'blob']
    for entry in files:
        local = args.source / entry['path']
        if not local.is_file():
            missing.append(entry['path'])
        elif git_hash(local.read_bytes()) != entry['sha']:
            blob = get_json(api + '/git/blobs/' + entry['sha'])
            upstream = base64.b64decode(blob['content'])
            if local.read_bytes().replace(b'\r\n', b'\n') == upstream.replace(b'\r\n', b'\n'):
                line_endings.append(entry['path'])
            else:
                changed.append(entry['path'])
    report = dict(checked_at=datetime.now(timezone.utc).isoformat(),
                  source=str(args.source.resolve()), branch='ng', commit=sha,
                  commit_date=commit['commit']['committer']['date'],
                  commit_url=commit['html_url'], checked_files=len(files),
                  missing=missing, changed=changed, line_endings_only=line_endings,
                  passed=not missing and not changed)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
    raise SystemExit(0 if report['passed'] else 1)
