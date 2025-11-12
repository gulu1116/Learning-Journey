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
./cjson_speed 
```

若需读取指定 JSON 文件（如 person.json），可传入文件路径作为参数：
```bash
./cjson_speed ../person.json
```


测试结果：
```text
cjson encode time testing
cjson encode 100000 time, need time: 646ms

cjson decode time testing
cjson decode 100000 time, need time: 274ms
```
