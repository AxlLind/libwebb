#!/usr/bin/python3
import pymk
from pymk import Target, PhonyTarget
from pathlib import Path
from os import chdir

chdir(Path(__file__).parent)

BUILD_DIR = Path('out')
if build_dir := pymk.get_variable('BUILD_DIR'):
    BUILD_DIR = Path(build_dir).resolve().relative_to(Path(__file__).parent)

for d in ['obj', 'tests', 'bin']:
    (BUILD_DIR / d).mkdir(exist_ok=True, parents=True)

pymk.set_variable(
    AR='ar',
    CC='gcc',
    CFLAGS='-std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow -pthread -fPIC',
)

def obj_target(c_file: Path) -> Target:
    return Target(
        cmd='$CC $CFLAGS -Iinclude -c $SRC -o $OUTPUT',
        depends={'SRC': c_file},
        output=BUILD_DIR / 'obj' / c_file.with_suffix('.o').name
    )

def test_target(c_file: Path, dlib: Target) -> Target:
    return Target(
        cmd='$CC $CFLAGS -Iinclude -Isrc $SRC $DLIB -o $OUTPUT',
        depends={'SRC': c_file, 'DLIB': dlib},
        output=BUILD_DIR / 'tests' / c_file.with_suffix('').name
    )

def bin_target(c_file: Path, slib: Target) -> Target:
    return Target(
        cmd='$CC $CFLAGS -Iinclude -Isrc $SRC $SLIB -o $OUTPUT',
        depends={'SRC': c_file, 'SLIB': slib},
        output=BUILD_DIR / 'bin' / c_file.with_suffix('').name
    )

def lint_target(c_file: Path) -> Target:
    return PhonyTarget(
        name=f'clang-tidy-{c_file}',
        cmd=f'clang-tidy {c_file} -- -std=gnu99 -Isrc -Iinclude 2>/dev/null',
    )

files = sorted([*Path('src').glob('*'), *Path('bin').glob('*'), *Path('tests').glob('*'), *Path('include/webb').glob('*')])
objs = [obj_target(f) for f in sorted(Path('src').glob('*.c'))]

slib = Target(
    cmd='$AR rc $OUTPUT $OBJS',
    depends={'OBJS': objs},
    output=BUILD_DIR / 'libwebb.a',
)

dlib = Target(
    cmd='$CC -shared -o $OUTPUT $OBJS',
    depends={'OBJS': objs},
    output=BUILD_DIR / 'libwebb.so',
)

bins = [bin_target(f, slib) for f in sorted(Path('bin').glob('*.c'))]
tests = [test_target(f, dlib) for f in sorted(Path('tests').glob('*.c'))]

pymk.main([
    PhonyTarget('build', depends=[slib, dlib, *bins, *tests], help='Compile everything'),
    PhonyTarget('run', depends={'WEBB': next(b for b in bins if b.output.name == 'webb')}, cmd='./$WEBB .', help='Run the web server'),
    PhonyTarget('test', depends=tests, cmd=' && '.join('./' + str(t.output) for t in tests), help='Run all tests'),
    PhonyTarget('fmt', cmd=f'clang-format -style=file {" ".join(str(f) for f in files)} -i', help='Format all source files, in place'),
    PhonyTarget('fmt-ok', cmd=f'clang-format -style=file {" ".join(str(f) for f in files)} --dry-run -Werror', help='Verify formatting'),
    PhonyTarget('lint', depends=[lint_target(f) for f in files], help='Run clang-tidy linter'),
    PhonyTarget('clean', cmd=f'rm -rf {BUILD_DIR}', help='Remove all build artifacts')
])