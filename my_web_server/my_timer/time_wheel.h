//时间轮
//较为简单的时间轮:只有一个轮子
#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class tw_timer;

//绑定socket与定时器
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    tw_timer* timer;
};

//定时器类
class tw_timer
{
public:
    tw_timer( int rot, int ts ) 
    : next( NULL ), prev( NULL ), rotation( rot ), time_slot( ts ){}

public:
    int rotation;//记录定时器在时间轮上转多少圈后生效
    int time_slot;//记录定时器属于时间轮上哪个槽(对应的链表，下同)
    void (*cb_func)( client_data* );//定时器回调函数
    client_data* user_data;//客户数据
    tw_timer* next;//下一个tw_timer
    tw_timer* prev;//上一个
};

//时间轮
class time_wheel
{
private:
    static const int N = 60;//时间轮上的槽数
    static const int SI = 1; //时间轮转动间隔
    tw_timer* slots[N];//时间轮 slots[i]:第i+1个槽的头结点
    int cur_slot;//当前槽


public:
    time_wheel() : cur_slot( 0 )
    {
        for( int i = 0; i < N; ++i )
        {
            slots[i] = NULL;//初始化每个槽的头结点
        }
    }
    ~time_wheel()//销毁每个槽
    {
        for( int i = 0; i < N; ++i )
        {
            tw_timer* tmp = slots[i];//
            while( tmp )//销毁此槽中所有节点
            {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }
    
    //根据timeout创建一定时器，将其插入合适槽中
    tw_timer* add_timer( int timeout )
    {
        if( timeout < 0 )
        {
            return NULL;
        }
        int ticks = 0;
        
        //ticks:滴答数
        //根据传入定时器超时值，计算其在时间轮转动多少个滴答后背触发，
        //并将该滴答数存储与变量ticks中，如果待插入定时器的超时值小于时间轮槽间隔SI
        //则将ticks向上折合为1，否则将ticks向下折合为timeout/SI
        if( timeout < SI )
        {
            ticks = 1;
        }
        else
        {
            ticks = timeout / SI;
        }
        //计算待插入的定时器在时间轮转动多少圈后背触发 N:槽数
        int rotation = ticks / N;
        //计算待插入定时器插入哪个槽中
        int ts = ( cur_slot + ( ticks % N ) ) % N;

        //一个定时器类，其在时间轮上转rotation圈后生效，
        //属于时间轮上第ts个槽
        tw_timer* timer = new tw_timer( rotation, ts );

        //该槽尚且没有定时器类，该插入为头结点
        if( !slots[ts] )
        {
            printf( "add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot );
            slots[ts] = timer;
        }
        else//该槽已存在时间器类，插在其prev 该槽头改为新插入时间器类 timer
        {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    //删除指定时间器timer
    void del_timer( tw_timer* timer )
    {
        if( !timer )
        {
            return;
        }

        //time_slot:timer所在的时间槽数
        int ts = timer->time_slot;
        //如果timer为该槽的头部
        if( timer == slots[ts] )
        {
            //将该槽头部换为timer的next
            slots[ts] = slots[ts]->next;
            if( slots[ts] )//如果其next存在，将其prev置空
            {
                slots[ts]->prev = NULL;
            }
            //删除timer时间器类
            delete timer;
        }
        else//timer不为头部
        {
            timer->prev->next = timer->next;//将timer的prev的next改为timer的next
            if( timer->next )//若timer的next不为空
            {
                //将其prev改为timer的prev
                timer->next->prev = timer->prev;
            }
            //删除timer时间器类
            delete timer;
        }
    }

    //每SI时间间隔，调用该函数，时间轮前滚一个槽的间隔
    void tick()
    {
        //当前槽
        tw_timer* tmp = slots[cur_slot];
        printf( "current slot is %d\n", cur_slot );

        //遍历完一个槽，该槽所有节点圈数减一，若有圈数为0，执行其回调函数
        while( tmp )
        {
            printf( "tick the timer once\n" );
            //该时间器类的圈数减一 若其圈数大于0，不执行，转向其下一个节点
            if( tmp->rotation > 0 )
            {
                tmp->rotation--;
                tmp = tmp->next;
            }
            else//若其圈数等于0，说明其该执行了
            {
                //执行函数 user_data:时间器类绑定的客户端类，其中含有文件描述符sockfd
                //IP地址，存储空间:buf[BUFFER_SIZE]
                tmp->cb_func( tmp->user_data );
                //若tmp为该槽头部,将该tmp删除，将头部换为其next
                if( tmp == slots[cur_slot] )
                {
                    printf( "delete header in cur_slot\n" );
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if( slots[cur_slot] )
                    {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                }
                else//不为槽头部
                {
                    tmp->prev->next = tmp->next;
                    if( tmp->next )
                    {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        //当前槽位置加一+
        cur_slot = ++cur_slot % N;
    }

};

#endif
