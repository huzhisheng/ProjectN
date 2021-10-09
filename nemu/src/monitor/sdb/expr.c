#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <common.h>

static void test_expr();

enum {
  TK_NOTYPE = 256, TK_EQ,TK_PLUS,

  /* TODO: Add more token types */
  TK_NUM,TK_MINUS,TK_MULTI,TK_DIV,TK_LK,TK_RK,
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal
  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */
  {"^[1-9][0-9]*", TK_NUM},
  {"-", TK_MINUS},
  {"\\*", TK_MULTI},
  {"/", TK_DIV},
  {"\\(", TK_LK},
  {"\\)", TK_RK},
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }

  // TODO: 加载gen_expr产生的文件, 并进行测试
  test_expr();
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          default: {
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
          }
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

int check_parentheses(int p, int q){
  if(tokens[p].type != TK_LK) return false;

  int left_numbers = 0; // 左括号的个数

  int i;
  for(i=p; i<=q; i++){
    if(tokens[i].type == TK_LK){
      left_numbers++;
    }
    else if(tokens[i].type == TK_RK){
      left_numbers--;
      if(i < q && left_numbers <= 0){
        return false;
      }
    }
  }
  if(left_numbers == 0) return true;
  else return false;
}

int find_main_op(int p, int q){
  int in_brackets = 0;

  int ret = -1;

  int i;
  for(i = p; i <= q; i++){
    switch (tokens[i].type)
    {
    case TK_PLUS:
    case TK_MINUS:
      {
        if(!in_brackets){
          ret = i;
        }
      }
      break;
    case TK_MULTI:
    case TK_DIV:
      {
        if(ret == -1 && !in_brackets){
          ret = i;
        }
      }
      break;
    case TK_LK:
      {
        in_brackets++;
      }
      break;
    case TK_RK:
      {
        in_brackets--;
      }
      break;
    default:
      break;
    }
  }
  return ret;
}

word_t eval(int p, int q) {
  printf("eval: %d, %d\n", p, q);
  if (p > q) {
    /* Bad expression */
    printf("err1\n");
    return -1;  // TODO: 返回值全1代表出错, 这步可能会有隐患
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a number.
     * Return the value of the number.
     */
    if(tokens[p].type != TK_NUM){ // 非数字
      printf("err2\n");
      return -1;
    }
    return atoll(tokens[p].str);
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1);
  }
  else {
    int op_index = find_main_op(p, q);
    if(op_index == -1){
      printf("err3\n");
      return -1;
    }
    word_t val1 = eval(p, op_index - 1);
    word_t val2 = eval(op_index + 1, q);

    switch (tokens[op_index].type) {
      case TK_PLUS: return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_MULTI: return val1 * val2;
      case TK_DIV: {
        if(val2 == 0) return 0; // TODO: 除以0则代表是非法表达式, 暂时就认为结果为0
        return val1 / val2;
      }
      default: printf("err4\n");return -1;
    }
  }
}


word_t expr(char *e, bool *success) {
  printf("开始计算表达式:%s\n",e);
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // TODO: 打印所有识别的东西
  for(int i=0; i<nr_token; i++){
    printf("第%d个token: %s\n", i, tokens[i].str);
  }

  /* TODO: Insert codes to evaluate the expression. */

  word_t ret_val = eval(0, nr_token-1);

  if(ret_val == -1){
    printf("Error in expr.\n");
    return 0;
  }

  return ret_val;
}

// TODO: 加载gen_expr产生的文件, 并进行测试
static void test_expr(){
  char filename[] = "/tmp/.code.input"; //文件名
  char *temp_p;
  FILE *fp; 
  char StrLine[1024]; //每行最大读取的字符数
  if((fp = fopen(filename,"r")) == NULL){ //判断文件是否存在及可读
      printf("Error in test_expr.\n"); 
      return; 
  } 

  while (!feof(fp)) 
  { 
      temp_p = fgets(StrLine,1024,fp); //读取一行
      if(temp_p == NULL){
        printf("Error in test_expr.\n"); 
        return; 
      }

      StrLine[strlen(StrLine) - 1] = '\0'; // 必须要忽略最后的换行符

      char *result_ptr = strtok(StrLine, " ");
      char *expr_ptr = strtok(NULL, " ");
      bool if_success = false;
      word_t expect_result = atoll(result_ptr);
      word_t actual_result = expr(expr_ptr, &if_success);

      printf("%ld, 计算结果:%ld\n", expect_result, actual_result);
  } 
  fclose(fp); //关闭文件
  return; 
}