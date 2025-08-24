import os
import subprocess
import sys
import time # 导入 time 模块

# --- 配置 ---
# 定义构建目录和可执行文件的名称
BUILD_DIR = "build"
EXECUTABLE_NAME = "maze_generator_app.exe"
PROJECT_ROOT = os.getcwd() # 获取当前工作目录 (应该由 build.sh 设置为项目根目录)

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

def main():
    """主构建函数"""
    start_time = time.time() # --- 新增: 记录开始时间 ---
    
    print(f"项目根目录: {PROJECT_ROOT}")

    # --- 步骤 1: 检查并创建构建目录 ---
    build_path = os.path.join(PROJECT_ROOT, BUILD_DIR)
    if not os.path.exists(build_path):
        print(f"构建目录 '{build_path}' 不存在，正在创建...")
        os.makedirs(build_path)
    else:
        print(f"构建目录 '{build_path}' 已存在。")

    # --- 步骤 2: 配置项目 (CMake) ---
    # CMake 命令需要从构建目录内部运行，并指向上一级目录 (项目根目录)
    cmake_command = [
        "cmake",
        "-G", "MSYS Makefiles",
        "-DCMAKE_BUILD_TYPE=Release",
        ".."
    ]
    run_command(cmake_command, working_dir=build_path)

    # --- 步骤 3: 构建项目 (make) ---
    # make 命令也需要在构建目录中运行
    make_command = ["make"]
    run_command(make_command, working_dir=build_path)

    end_time = time.time() # --- 新增: 记录结束时间 ---
    duration = end_time - start_time # --- 新增: 计算总耗时 ---

    # --- 步骤 4: 显示最终信息 ---
    executable_path = os.path.join(build_path, EXECUTABLE_NAME)
    print("\n" + "="*30)
    print("构建完成!")
    print(f"可执行文件已生成在: {executable_path}")
    print(f"你可以运行它: ./{os.path.join(BUILD_DIR, EXECUTABLE_NAME)}")
    print(f"总耗时: {duration:.2f} 秒") # --- 新增: 显示总耗时 ---
    print("="*30)


if __name__ == "__main__":
    main()
