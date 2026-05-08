#ifndef RECORD_H_
#define RECORD_H_

#include "meta.h"
#include "datypes.h"
#include "netypes.h"
#include "buffer.h"
#include "logical.h"
#include <unordered_map>

typedef struct {
    int16_t id;   // 排序字段id
    int16_t type; // 排序字段类型，1升 2降
} order_t;

typedef struct {
    int16_t aid; // 聚合函数ID
    int16_t fid; // 字段ID
} agg_t;

// 读取数据，请求体
typedef struct {
    vector<int16_t> fields; // 读取字段，例如: code,name,close
    vector<string>  keys;   // 指定key，例如：000001.SZ,600519.SH
    string          where;
    vector<order_t> order;
    vector<agg_t>   aggs;   // 聚合函数: count/sum/avg/max/min，分别用数字12345代替
    vector<int16_t> group;  // 根据那些字段分组，group by xx,yy,zz
    int             offset; // limit 起点
    int             size;   // limit 数据大小，-1代表全部
} request_t;

class Record {
    public:
        Record(Meta *m);
        ~Record();

        /*
         * 对外提供7种操作：，返回字节数，如果为-1，说明有错误
         *   OPT_TEST:   opt_test()
         *   OPT_CONF:   opt_conf()
         *   OPT_COLUMN: opt_column()
         *   OPT_DETAIL: opt_detail()
         *   OPT_DUMP:   opt_dump()
         *   OPT_SET:    opt_set()
         *   OPT_GET:    opt_get()
         *   OPT_AGG:    opt_agg()
         *   OPT_DEL:    opt_del()
         */

        // 测试模式，客户端发送x字节，服务端返回x字节
        bool opt_test(buffer_t *recv, buffer_t *send) const;

        /*
         * 除opt_test()外，所有数据返回格式统一为：[int4][int4][数据体]
         *  第1个int：表示数据长度，= 4 + length(数据体)
         *  第2个int，表示成功OK或失败ERROR。如果失败，数据体存储错误信息，统一格式：[int2][错误字符串]
         *  
         *  数据总长度 = 8 + length(数据体)
         *  
         */

        // 所有函数都是流式读取，流式写入，减少内存复制

        // 获取服务器配置信息
        bool opt_conf(buffer_t *recv, buffer_t *send) const;

        // 获取字段信息，写入output
        bool opt_column(buffer_t *recv, buffer_t *send) const;

        // 获取字段详细信息
        bool opt_detail(buffer_t *recv, buffer_t *send) const;

        // 备份数据
        bool opt_dump(buffer_t *recv, buffer_t *send) const;

        // 更新数据
        // 接收到的数据格式：[头信息][size][key][f1][f2][f3]...[size][key][f1][f2]..
        bool opt_set(buffer_t *recv, buffer_t *send);

        // 读取数据
        bool opt_get(buffer_t *recv, buffer_t *send);

        // 统计数据
        bool opt_agg(buffer_t *recv, buffer_t *send);
        
        // 删除数据
        bool opt_del(buffer_t *recv, buffer_t *send);
        
        // 备份数据，返回条数
        int dump() const;

        // print所有数据
        void data() const;

        // 数据条数
        int size() const;

    private:
        // 启动时从dump文件读取数据
        void load();

        // 计算表达式
        double formula_compute(const char *value, const Expression& ex);

        // 数据更新
        bool setone(const string& key, const char *byte, int size);

        unordered_map<string, char *> data_; // 数据，全内存
        Meta *meta_;    // 配置
        Computer computer_; // 用于公式计算

        // 将所有字段拼接成buffer，发给每一个客户端
        void put_field_to_buffer();
        char *field_buffer_;
        int   field_buffer_size_;

        // 读取数据
        request_t request_;
        bool parse_get_request(char *buff, int size);
        bool parse_del_request(char *buff, int size);
        bool parse_agg_request(char *buff, int size);

        vector<char *> read_data_; // 符合条件的数据

        // where条件
        Logical lg_; // 逻辑表达式
        bool load_and_check_logical(const string& where, string *errmsg);
        bool compute_logical(const char *value);
        // 计算逻辑表达式需要用到stack，以下2个变量用于逻辑计算
        // 如果条件中存在连续128个左括号，数组会溢出，返回false
        char lg_stack_[128];
        int  lg_stack_size_;
        void put_stack(char c);

        // 排序比较函数，对read_data_排序
        bool compare(const char *a, const char *b) const;

        // 聚合函数，根据groupby字段构建key
        void make_key_by_group(const char *value, string *key);
};

#endif // RECORD_H_
