# xv6-lottery_schedule
xv6においてlottery_schedulerを実装する。
## 結果
チケット数の異なる５つのスレッドを並行して500秒間実行したところ、以下のような結果が得られた。
割り振られたチケット数に応じて、500秒間の間にスレッドが呼び出された回数が異なるのが確認できる。

```
$ test_sys
pid: 5, ticket_num: 10, called: 359
pid: 6, ticket_num: 20, called: 672
pid: 7, ticket_num: 30, called: 965
pid: 8, ticket_num: 40, called: 1379
pid: 9, ticket_num: 50, called: 1645

```
## lottery_schedulerの実装
lottery_schedulerを実装する上で改良した部分を以下に示す。

### proc.h

proc構造体に、チケット数を格納する`tickets`を追加。
`called_counter`はスケジューラーの評価時にプロセスが呼び出された回数を計測するために追加。
```c
struct proc {
  （省略）
  int tickets;                 // ticket number added
  int called_counter;           // called counter added
};
```
### proc.c
**・alloc_proc()**

プロセス生成時に`tickets`を０で初期化する処理を追加。
また、スケジューラー評価のために`called_couter`も初期化。
```c
static struct proc*
allocproc(void)
{
  （省略）
found:
  p->pid = allocpid();
  p->state = USED;
  p->tickets = 10; //set the default ticket number
  p->called_counter = 1; //set the called counter

 （省略）
  }
```


**・freeproc()**

プロセスの解放時に`tickets`と`called_couter`を０で初期化。
```c
static void
freeproc(struct proc *p)
{
  （省略）
  p->tickets = 0;
  p->called_counter = 0;
  （省略）
}
```


**・total_tickets_num**

プロセスリスト内の現在`RUNNABLE`な状態のプロセスのチケット数の合計を返す関数。
```c
//calculate the total tickets number
int total_ticket_num(void){
  struct proc *p;
  int sum_tickets = 0;
  for(p = proc; p < &proc[NPROC];p++){
    if(p->state == RUNNABLE){
      sum_tickets += p->tickets;
    }
  }
  return sum_tickets;
}
```


**・scheduler()**

lottery_schedulerのメインとなる部分。`winner_ticket`には、０〜`total_tickets−１`までの乱数が格納される。

無限ループのはじめにチケットの総数の取得と、乱数の取得の処理を行う。

その後のfor文内で`RUNNABLE`なプロセスのチケットを`coutenr`に足していき、その値が`winner_ticket`を超えた時点でそのプロセスにスイッチ。

プロセスが`RUNNABLE`でない場合や、選ばれたプロセスでなかった場合は、lockを解放し、continue命令でfor文の先頭に戻る。
```c
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  //////
  int total_tickets = 0; //number of the sum of tickets
  int counter = 0;
  int winner_ticket; //random number
  //////
  c->proc = 0;
  for(;;){
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    //resetting the variables to make scheduler start from the beginning of the process queue
    total_tickets = 0;
    counter = 0;
    winner_ticket = 0;;

    //calculate total number of tickets
    total_tickets = total_ticket_num();

    //pick a random ticket from total available ticket
    winner_ticket = random_at_most(total_tickets);

    for(p = proc; p < &proc[NPROC]; p++) {
      acquire(&p->lock);
      if(p->state != RUNNABLE){
        release(&p->lock); //added
        continue;
      }
      counter += p->tickets;
      if(counter < winner_ticket){
        release(&p->lock); //added
        continue;
      }
        
      // Switch to chosen process.  It is the process's job
      // to release its lock and then reacquire it
      // before jumping back to us.
      p->called_counter += 1; //increment called counter
      p->state = RUNNING;
      c->proc = p;
      swtch(&c->context, &p->context);

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
      release(&p->lock);
      break; //reset the location in the list
    }
  }
}
```

### rand.c
乱数の生成を行うプログラム。`random_at_most()`により、lottery_schedulerにおける、指定値以下の乱数を取得する処理を実現。
```c
 （省略）
// Assumes 0 <= max <= RAND_MAX
// Returns in the half-open interval [0, max]
long random_at_most(long max) {
  unsigned long
    // max <= RAND_MAX < ULONG_MAX, so this is okay.
    num_bins = (unsigned long) max + 1,
    num_rand = (unsigned long) RAND_MAX + 1,
    bin_size = num_rand / num_bins,
    defect   = num_rand % num_bins;

  long x;
  do {
   x = genrand();
  }
  // This is carefully written not to overflow
  while (num_rand - defect <= (unsigned long)x);

  // Truncated division is intentional
  return x/bin_size;
}
```

## スケジューラーの評価
スケジューラーが正しく動作するかどうかを調べるためにユーザプログラムを作成。
### test_sys.c
チケット数が10、20、30、40、50の異なる５つのプロセスを並行して500秒間実行。

最終的に、それぞれのプロセスが500秒間の間にどれだけ呼ばれたかを出力。

```c
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

```

新たに２つのシステムコールを実装。

**・change_tickets()**

プロセスのチケット数を引数で受け取った値に変更するシステムコール
```
uint64
sys_change_tickets(void){
  int num;
  argint(0,&num);
  myproc()->tickets = num;
  return 0;
}
```

**・print_info()**

プロセスのPID、チケット数、呼び出された回数の３つの情報を出力するシステムコール。

```
uint64
sys_print_info(void){
  struct proc *p;
  p = myproc();
  printf("pid: %d, ticket_num: %d, called: %d\n",p->pid,p->tickets,p->called_counter);
  return 0;
}
```
