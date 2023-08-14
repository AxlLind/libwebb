import sys
import subprocess
from concurrent.futures import ProcessPoolExecutor

def lint(f: str, args: list[str]) -> bool:
    res = subprocess.run(['clang-tidy', f, *args], stderr=subprocess.DEVNULL)
    return res.returncode == 0

def main(args: list[str]) -> int:
    i = args.index('--') if '--' in args else len(args)
    files, args = args[:i], args[i:]
    with ProcessPoolExecutor() as executor:
        results = list(executor.map(lint, files, [args] * len(files)))
    return 0 if all(results) else 1

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
