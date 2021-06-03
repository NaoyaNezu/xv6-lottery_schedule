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
/* A C-program for MT19937: Real number version                */
/*   genrand() generates one pseudorandom real number (double) */
/* which is uniformly distributed on [0,1]-interval, for each  */
/* call. sgenrand(seed) set initial values to the working area */
/* of 624 words. Before genrand(), sgenrand(seed) must be      */
/* called once. (seed is any 32-bit integer except for 0).     */
/* Integer generator is obtained by modifying two lines.       */
/*   Coded by Takuji Nishimura, considering the suggestions by */
/* Topher Cooper and Marc Rieffel in July-Aug. 1997.           */

/* This library is free software; you can redistribute it and/or   */
/* modify it under the terms of the GNU Library General Public     */
/* License as published by the Free Software Foundation; either    */
/* version 2 of the License, or (at your option) any later         */
/* version.                                                        */
/* This library is distributed in the hope that it will be useful, */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of  */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.            */
/* See the GNU Library General Public License for more details.    */
/* You should have received a copy of the GNU Library General      */
/* Public License along with this library; if not, write to the    */
/* Free Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA   */ 
/* 02111-1307  USA                                                 */

/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */

/* Period parameters */  
#define N 624
#define M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */   
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

#define RAND_MAX 0x7fffffff

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializing the array with a NONZERO seed */
void
sgenrand(unsigned long seed)
{
    /* setting initial seeds to mt[N] using         */
    /* the generator Line 25 of Table 1 in          */
    /* [KNUTH 1981, The Art of Computer Programming */
    /*    Vol. 2 (2nd Ed.), pp102]                  */
    mt[0]= seed & 0xffffffff;
    for (mti=1; mti<N; mti++)
        mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

long /* for integer generation */
genrand()
{
    unsigned long y;
    static unsigned long mag01[2]={0x0, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= N) { /* generate N words at one time */
        int kk;

        if (mti == N+1)   /* if sgenrand() has not been called, */
            sgenrand(4357); /* a default initial seed is used   */

        for (kk=0;kk<N-M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (;kk<N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }
  
    y = mt[mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    // Strip off uppermost bit because we want a long,
    // not an unsigned long
    return y & RAND_MAX;
}

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
### -test_sys.c
