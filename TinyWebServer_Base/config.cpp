#include "config.h"

Config::Config()
{
    //端口号,默认9006
    PORT = 9006;

    //触发组合模式,默认listenfd LT + connfd LT
    TRIGMode = 1;

    //listenfd触发模式，默认LT
    LISTENTrigmode = 0;

    //connfd触发模式，默认LT
    CONNTrigmode = 0;

    //数据库连接池数量,默认8
    sql_num = 8;

    //线程池内的线程数量,默认8
    thread_num = 8;

    //并发模型,默认是reactor   0是proactor,1是reactor，
    actor_model = 1;
}

void Config::parse_arg(int argc, char*argv[])
{
//    int opt;
//    const char *str = "p:l:m:o:s:t:c:a:";
//    while ((opt = getopt(argc, argv, str)) != -1)
//    {
//        switch (opt)
//        {
//            case 'p':
//            {
//                PORT = atoi(optarg);
//                break;
//            }
//            case 'm':
//            {
//                TRIGMode = atoi(optarg);
//                break;
//            }
//            case 's':
//            {
//                sql_num = atoi(optarg);
//                break;
//            }
//            case 't':
//            {
//                thread_num = atoi(optarg);
//                break;
//            }
//            case 'a':
//            {
//                actor_model = atoi(optarg);
//                break;
//            }
//            default:
//                break;
//        }
//    }
}
