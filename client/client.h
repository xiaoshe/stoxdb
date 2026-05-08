#ifndef CLIENT_H_
#define CLIENT_H_

#include <string>
#include <vector>
#include <map>
#include "../datypes.h"
#include "../netypes.h"
#include "../buffer.h"

using namespace std;

// 写入时，需要指定id，检查类型，检查字符串是否越界
struct Simple {
    int32_t id;
    int16_t type;
    int16_t sz;
};

struct Request {
    string fields;
    string keys;
    string where;
    string order;
    int    offset;
    int    size;
    Request() : offset(0), size(-1) {}
};

class Client {
    public:
        Client(const string& host="127.0.0.1", int port=30634, int magic=41401891);
        ~Client();

        // 判断是否响应失败
        bool Error() const;

        // 服务器配置信息
        bool Conf();

        // 服务器状态
        bool Status();

        // 字段列表
        bool Column(vector<Field> *out);

        // 详细字段
        bool Detail(vector<Field> *out);

        // 备份数据，返回条数
        int Dump();

        // 更新数据
        Simple getid(const string& name) const;
        void begin();
        void end();
        // setkey表示某条数据的开始，先setkey，再setvalue
        void setkey(const string& k);
        void setvalue(const string& name, const string& v);
        void setvalue(const string& name, int64_t v);
        void setvalue(const string& name, int32_t v);
        void setvalue(const string& name, double v);

        // 读取数据
        bool get(const Request& req);

    private:
        bool Connect();
        bool UpdateFields();

        string host_;
        int port_;
        int magic_;
        int sockfd_;
        map<string, Simple> fname_map_; // 字段名称 => {id, type}，用于数据更新
        vector<Field> fields_;

        void SetHeader(int size, int opt);
        bool Send(); // 发送缓冲区数据
        bool Recv(); // 接收数据到缓冲区

        buffer_t buffer_; // 缓冲区

        // 接收数据信息
        int size_;  // 数据大小，不含头
        int status_;// 状态，OK/ERROR
        char *data_;// 数据开始位置，data_指向buffer+8

        char *begin_; // 更新时使用，指向buffer某个位置
        char *end_;

        char *output(const vector<int16_t>& fids, const vector<string>& fnames, char *p);
};

#endif // CLIENT_H_
