# qqwry2txt
`qqwry2txt` 仅用于解析[纯真 IP 库镜像](https://github.com/wisdomfusion/qqwry.dat)到 utf-8 编码的文件中, 代码参考[ C 解析 QQwry.dat](https://www.iteye.com/blog/hzy3774-1851364)

```
Options:
-f <file>       纯真 IP 库文件位置
-o <file>       解析后的输出位置, 默认 "./output.txt"
-h, --help      Display this information

例:
./qqwry2txt -f ../data/qqwry.dat -o ../data/qqwry_output.txt

 3.996801s
```

# iplib_maker
`iplib_maker` 读取 `qqwry2txt` 生成的 txt (单行内容示例:"42.158.0.0|42.158.255.255|<描述内容>"), 构建 IP 库文件, 相较于[原始数据](https://github.com/wisdomfusion/qqwry.dat) 具有性能优势.
数据结构参考 [ip2region](https://github.com/lionsoul2014/ip2region), 删除了头部跳表 (索引区元素大小固定, 且有序, 此跳表意义不大), 同时删除了 "city_id" 这样的字段, 仅保留 key (IP 区间) - value (描述内容), 复杂内容可转为 json 存入 value, 最大支持长度为 65535 字节

```
iplib 数据结构:
    所有长度偏移等整数信息均为小端序(Little-Endian), 为的是直接内存读取

1. 文件头:
  +------------+------------+------------+------------+
  | 4 bytes    | 4 bytes    | 4 bytes    | 4 bytes    |
  +------------+------------+------------+------------+
    "PUNK"     | 索引区开始 | 索引区结束 | 保留字段

2. 数据区: 仅当数据索引区 "数据长度" 字段等于 0xFF 时, 数据区的 "数据长度"
字段才会存在
  +---------+----------------------+
  | 2 bytes | max 65535 bytes      | ...
  +---------+----------------------+
   数据长度 | 描述字符串

3. 数据索引区:
  +------------+-----------+--------+---------+
  | 4 bytes    | 4 bytes   | 1 byte | 3 bytes | ...
  +------------+-----------+--------+---------+
   start ip    |  end ip   |数据长度| 数据偏移

```
```
Options:
-f <file>	Source text file path
-o <file>	Write output to <file>; default "./iplib.db"
-h, --help	Display this information

例:
./iplib_maker -f ../../data/qqwry_output.txt -o ../../data/iplib.db
done.

 2.360622s
```

# iplib_reader
`iplib_reader` 用于读取 `iplib_maker` 生成的 IP 库文件, 同时支持读取 [ip2region IP 库](https://github.com/lionsoul2014/ip2region), 查询特定 IP 地址的关联信息

代码参考 [ip2region C 客户端](https://github.com/lionsoul2014/ip2region/tree/master/binding/c)

```
Options:
-f <file>       IP library file path
-s <ip>         The IP address to search
-h, --help      Display this information

例:
./iplib_reader -f ../../data/iplib.db -s 124.250.236.20
'北京市|世纪互联数据中心' 0.01196 ms
```

