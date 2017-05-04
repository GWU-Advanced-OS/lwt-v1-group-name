# Trade-offs and implementation
## Lwt isolation
We create an idle lwt for each kthd, when it is created. This idle lwt can be the head and entry of the lwt queue of this kthd. However, in this condition, if the preemption happens, the lwt_head which is a global variable will be modified, which means when switching back to this kthd, lwt_head may be a lwt of another kthd. So we decided to add critical section to our kthd.

## Inter-kthd Comunication
We choose to make the channel support both local and remote communication. When a rcv request is made, if there is no data in the channel, this lwt will block itself to wait for the snd. If it is inner-kthd communication, the performance should be decent. However, if it is inter-kthd communication, many performance will be wasted on context switch.

## Synchronization
Since preemption may happen, we need to make sure that global or shared variable like global counter and ring_buffer of each channel should be safely operated, so we add cas to these variable to make sure the security of them.

## Test
model name	: Intel(R) Core(TM) i5-4258U CPU @ 2.40GHz

|     |                System&Param                |      context_switch(per thread)      | pipe/channel(per thread)  | proc/thd_create(per thread)  | event_notification(per thread)  |
|-----|:------------------------------------------:|:------------------------:|:------------:|:---------------:|:------------------:|      |
|           linux/lmbench(time:ms)           |      54.25   (1000threads)        |    51.2161(2 process)    |     188.2574    |       1.2477(100files)|
| 2   |             lwt_local hw3               |           43          |      150     |       233      |         2000       |
| 3   |             lwt_local                   |          5071         |     932     |       7290      |        74490        |
| 4   |             lwt_remote                  |         822071         |     415394    |      136831      |        12,119       |

# Golang
##Creating goroutines
With golang, people can just add a “go” before that function to make it a goroutine, and then it will be scheduled. The goroutinue is pretty light-weight, since it just have a 4K stack ,a ip and a sp, which are very similar to our lwt. However, in contrast to our lwt library, it is easier to create a light weight thread using golang.

##using channel for communication
After searching the information about golang, we found channel and goroutine can be the most significant features of golang. In general, the channel share the same design ideas with our lwt_channel. When the buffer size of the channel is 0, communication between threads will be synchronous. If the size is larger than 0, the sender will not be blocked until the buffer is full. But it is easier to use the channel in go language. For example, “msg := <-c” is the receive code, which looks quite short and clear.

##multi-wait
Although we carefully searched the Internet about the channel group in golang, we didn’t find any information about this.  Firstly, we just found the waitgroup function, which is used to block the program to wait for goroutines. Then, we found the select statement. If no channels are readly, the statement blocks until one becomes available. If more than one of the channels are ready, it just randomly picks a channel to receive. This looks really our implementation of channel group. However, it is still slightly different. We pick the event following the “FIFO” rules. Not only that, channels in the select statement are fixed, and our cgrp can dynamically add and remove channels.

##Concurrency
In golang scheduler, we have three components, which are M, P, G. M,P,G represent kernel thread, context, and goroutine respectively. The number of P controls the degree of concurrency. Each P maintains a goroutine queue. And the G runs on the M. If the M0 does a syscall and is blocked, The P of M0 will move to another M like M1, which could be an idle M or just be created.  When M0 woke up, it will try to get a P from other M. If it doesn’t get the P, it will put its goroutine in a global runqueue, and then get back to the thread pool. P will periodically check the global runqueue to make sure these goroutines can be executed.
Decision about preemption can be made by the sysmon() background thread. This function will periodically do epoll operation and check if each P runs for too long. It it find a P has been in Psyscal state for more a sysmon time period and there is runnable task, it will switch P. If it find a P in Prunning state running for more than 10ms, it will set the stackguard of current G of P StackPreempt to let the G know it should give up the running opportunity.
