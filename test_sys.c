#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int start_tick;

void child_process(int ticket_num){
	int end_tick;
	change_tickets(ticket_num);
	for(;;){
		end_tick = uptime();
		if((end_tick - start_tick)/10 > 500)
			break;
	}
	print_info(); //print {pid, amount of ticket, number of calls}
}

int
main(int argc, char *argv[]){
	int max_ratio = 5,pid;
	start_tick = uptime();
	for(int i=1;i<=max_ratio;i++){
		pid = fork();
		if(pid == 0){
			child_process(i*10);
			break;
		}
	}
	wait(0);
	exit(0);
}
