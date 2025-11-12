CMake（版本 ≥ 3.10）
C 编译器（如 GCC、Clang）

在项目根目录下创建并进入 build 目录（用于分离源码与构建文件）：
```bash
mkdir -p build && cd build
```

执行 cmake 命令生成 Makefile（或其他构建系统文件）
```bash
cmake ..
```

执行 make 命令编译生成可执行文件：
```bash
make
```

编译完成后，在 build 目录下执行可执行文件：
```bash
./rapidjson_speed
```


测试结果：
```text
rapidjson encode 100000 time, need time: 105ms
rapidjson decode time testing
rapidjson decode 100000 time, need time: 167ms

jsoncpp decode time testing
jsoncpp decode 100000 time, need time: 1301ms
```
