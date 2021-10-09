#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>

// this should be enough
static char buf[65536] = {};
// TODO: 添加一个buf长度变量
static int buf_len = 0;

static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";

// TODO:
static int choose(int num_choices)
{
  return rand()%num_choices;    //用rand产生随机数并设定范围
}
// TODO:
static void gen_num(){
  int rand_num = choose(1000);
  char num_buf[5];
  sprintf(num_buf, "%d", rand_num);
  int num_str_len = strlen(num_buf);
  memcpy(buf+buf_len, num_buf, num_str_len);
  buf_len += num_str_len;
}
// TODO:
static void gen(char e){
  buf[buf_len++] = e;
  return;
}
// TODO:
static void gen_rand_op(){
  switch (choose(5)) {
    case 0: buf[buf_len++] = '+'; break;
    case 1: buf[buf_len++] = '-'; break;
    case 2: buf[buf_len++] = '*'; break;
    default: buf[buf_len++] = '/'; break;
  }
}

// TODO:
static void gen_rand_expr() {
  switch (choose(5)) {
    case 0:
    case 1:
    case 2: gen_num(); break;
    case 3: gen('('); gen_rand_expr(); gen(')'); break;
    default: gen_rand_expr(); gen_rand_op(); gen_rand_expr(); break;
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf_len = 0;  // TODO: 清零buf_len

    gen_rand_expr();

    if(buf_len >= 65500) continue;  // TODO: 生成的表达式太长, 跳过
    buf[buf_len] = '\0';

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
