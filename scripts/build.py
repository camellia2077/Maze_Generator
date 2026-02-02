import argparse
import json
import os
import subprocess
import sys
import time # 导入 time 模块

# --- 配置 ---
# 定义构建目录和可执行文件的名称
BUILD_DIR_RELEASE = "build"
BUILD_DIR_DEBUG = "build_debug"
EXECUTABLE_NAME = "maze_generator_app.exe"
PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

def run_command(command, working_dir):
    """一个辅助函数，用于在指定的目录中运行命令并检查错误"""
    print(f"--- 在 '{working_dir}' 中执行: {' '.join(command)} ---")
    try:
        # 使用 subprocess.run 来执行命令
        # check=True 会在命令返回非零退出码时抛出异常，类似于 'set -e'
        subprocess.run(command, check=True, cwd=working_dir)
        print("--- 命令成功 ---")
    except FileNotFoundError:
        print(f"错误: 命令 '{command[0]}' 未找到。请确保它已安装并在您的 PATH 中。")
        sys.exit(1) # 退出脚本
    except subprocess.CalledProcessError as e:
        print(f"错误: 命令 '{' '.join(command)}' 失败，退出码 {e.returncode}")
        sys.exit(1) # 退出脚本

def normalize_path(path):
    return os.path.normcase(os.path.normpath(path))

def collect_tidy_sources(compile_commands_path):
    with open(compile_commands_path, "r", encoding="utf-8") as handle:
        compile_commands = json.load(handle)

    project_root_slash = PROJECT_ROOT.replace("\\", "/")
    src_root_slash = f"{project_root_slash}/src/"
    excluded_segments = ["/src/vendor/"]

    sources = []
    seen = set()
    for entry in compile_commands:
        file_path = entry.get("file")
        if not file_path:
            continue

        file_path_slash = file_path.replace("\\", "/")
        if not file_path_slash.startswith(src_root_slash):
            continue
        if any(segment in file_path_slash for segment in excluded_segments):
            continue

        normalized = normalize_path(file_path)
        if normalized in seen:
            continue

        seen.add(normalized)
        sources.append(file_path)

    return sources

def run_clang_tidy_per_file(build_path):
    compile_commands_path = os.path.join(build_path, "compile_commands.json")
    if not os.path.exists(compile_commands_path):
        print(f"错误: 未找到编译数据库: {compile_commands_path}")
        print("请先完成 CMake 配置，确保生成 compile_commands.json。")
        sys.exit(1)

    sources = collect_tidy_sources(compile_commands_path)
    if not sources:
        print("未找到需要检查的源文件。")
        return

    src_root_slash = os.path.join(PROJECT_ROOT, "src").replace("\\", "/")
    header_filter = f"^{src_root_slash}/(?!vendor/)"
    total = len(sources)
    failures = 0

    for index, source in enumerate(sources, start=1):
        print(f"[{index}/{total}] clang-tidy: {source}", flush=True)
        tidy_command = [
            "clang-tidy",
            "-p", build_path,
            f"-header-filter={header_filter}",
            "-fix",
            "-format-style=file",
            source,
        ]
        result = subprocess.run(tidy_command, cwd=PROJECT_ROOT)
        if result.returncode != 0:
            failures += 1

    if failures > 0:
        print(f"clang-tidy 失败文件数: {failures}")
        sys.exit(1)

def parse_args():
    parser = argparse.ArgumentParser(description="Build MazeGenerator")
    opt_group = parser.add_mutually_exclusive_group()
    opt_group.add_argument(
        "--opt",
        dest="enable_optimizations",
        action="store_true",
        help="Enable release optimizations",
    )
    opt_group.add_argument(
        "--no-opt",
        dest="enable_optimizations",
        action="store_false",
        help="Disable release optimizations",
    )
    parser.set_defaults(enable_optimizations=True)
    parser.add_argument(
        "--target",
        choices=["all", "format", "tidy"],
        default="all",
        help="Build target",
    )
    parser.add_argument(
        "--build-type",
        choices=["Release", "Debug"],
        default="Release",
        help="CMake build type",
    )
    parser.add_argument(
        "--build-dir",
        default=None,
        help="Build directory (overrides default by build type)",
    )
    parser.add_argument(
        "--generator",
        default=None,
        help="CMake generator (optional)",
    )
    return parser.parse_args()


def main():
    """主构建函数"""
    start_time = time.time() # --- 新增: 记录开始时间 ---
    args = parse_args()
    
    print(f"项目根目录: {PROJECT_ROOT}")

    # --- 步骤 1: 检查并创建构建目录 ---
    default_build_dir = BUILD_DIR_DEBUG if args.build_type == "Debug" else BUILD_DIR_RELEASE
    build_dir = args.build_dir if args.build_dir else default_build_dir
    build_path = os.path.join(PROJECT_ROOT, build_dir)
    if not os.path.exists(build_path):
        print(f"构建目录 '{build_path}' 不存在，正在创建...")
        os.makedirs(build_path)
    else:
        print(f"构建目录 '{build_path}' 已存在。")

    # --- 步骤 2: 配置项目 (CMake) ---
    cmake_command = [
        "cmake",
        "-S", PROJECT_ROOT,
        "-B", build_path,
        f"-DCMAKE_BUILD_TYPE={args.build_type}",
        f"-DMAZE_ENABLE_OPTIMIZATIONS={'ON' if args.enable_optimizations else 'OFF'}",
    ]
    if args.generator:
        cmake_command.extend(["-G", args.generator])
    run_command(cmake_command, working_dir=PROJECT_ROOT)

    # --- 步骤 3: 构建项目 (CMake --build) ---
    if args.target == "tidy":
        run_clang_tidy_per_file(build_path)
    else:
        build_command = ["cmake", "--build", build_path]
        if args.target != "all":
            build_command.extend(["--target", args.target])
        run_command(build_command, working_dir=PROJECT_ROOT)

    end_time = time.time() # --- 新增: 记录结束时间 ---
    duration = end_time - start_time # --- 新增: 计算总耗时 ---

    # --- 步骤 4: 显示最终信息 ---
    print("\n" + "="*30)
    if args.target == "all":
        executable_path = os.path.join(build_path, EXECUTABLE_NAME)
        print("构建完成!")
        print(f"可执行文件已生成在: {executable_path}")
        print(f"你可以运行它: ./{os.path.join(build_dir, EXECUTABLE_NAME)}")
    else:
        print(f"目标 '{args.target}' 完成!")
    print(f"总耗时: {duration:.2f} 秒") # --- 新增: 显示总耗时 ---
    print("="*30)


if __name__ == "__main__":
    main()
