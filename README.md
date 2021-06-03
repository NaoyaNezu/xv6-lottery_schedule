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
を実装する上で改良した部分を以下に示す。
### proc.h
```
struct proc {
  （省略）
  int tickets;                 // ticket number added
  int called_counter;           // called counter added
};
```
### proc.c
### rand.c

## スケジューラーの評価
### test_sys.c
### proc.h
