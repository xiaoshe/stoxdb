#ifndef META_H_
#define META_H_

#include <unordered_map>
#include "datypes.h"

using namespace std;


class Meta {
    public:
        Meta();
        ~Meta();

        // 加载配置文件
        bool load_config();

        // 配置信息
        void info() const;

        // 字段信息
        void desc() const;

        // 根据字段名获取字段ID(>=0)，-1表示字段不存在
        int getid(const string& name) const;


    public:
        int port_;
        string dump_file_;
        int magic_number_;
        int max_recv_size_;
        int max_connections_;
        int idle_timeout_;
        int transport_timeout_;

        int record_size_;  // 每条记录的大小，是固定的，根据配置计算得来
        vector<Field> fields_; // 字段信息列表

    private:
        unordered_map<string, int> fname_id_; // 字段名与ID的映射

        // 判断公式中的字段是否存在环，如果存在环，在计算公式时，则将陷入死循环
        bool has_circle(int fid) const;
};

#endif
