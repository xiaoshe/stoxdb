#ifndef SERVER_H_
#define SERVER_H_

#include "connection.h"

class Meta;
class Record;
class Server {
    public:
        Server(Meta* meta, Record *rd);
        ~Server();
        bool init();
        void start(int *running);

    private:
        bool handle_read_event(connection_t *conn);
        bool handle_write_event(connection_t *conn);
        bool parse_header(connection_t *conn);
        bool process_data(connection_t *conn);
        Record *record_;
        Meta   *meta_;
};

#endif // SERVER_H_
