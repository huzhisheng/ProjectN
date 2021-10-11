#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <common.h>
#include <memory/paddr.h>

static void test_expr();

enum {
  TK_NOTYPE = 256, TK_EQ,TK_PLUS,

  /* TODO: Add more token types */
  TK_NUM,TK_MINUS,TK_MULTI,TK_DIV,TK_LK,TK_RK,
  TK_HEXNUM,TK_REG,TK_NEQ,TK_AND,TK_DEREF,
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
  
  {"-", TK_MINUS},
  {"\\*", TK_MULTI},
  {"/", TK_DIV},
  {"\\(", TK_LK},
  {"\\)", TK_RK},

  {"0[xX][0-9a-fA-F]+", TK_HEXNUM},
  {"^[0-9][0-9]*", TK_NUM},
  {"\\$x[0-9][0-9]?", TK_REG},
  {"!=", TK_NEQ},
  {"&&", TK_AND},
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

static Token tokens[256] __attribute__((used)) = {};  //! 将tokens的长度从32改为了256
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

        // Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
        //     i, rules[i].regex, position, substr_len, substr_len, substr_start);

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
  int op_level = 0; // 数字越小, 符号优先级越高
  //! 规定: 解引用的优先级为0, 乘除的优先级为1, 加减的优先级为2, 大小比较的优先级为3, 逻辑运算的优先级为4
  int i;
  for(i = p; i <= q; i++){
    switch (tokens[i].type)
    {
    case TK_PLUS:
    case TK_MINUS:
      {
        if(!in_brackets && op_level <= 2){
          ret = i;
          op_level = 2;
        }
      }
      break;
    case TK_MULTI:
    case TK_DIV:
      {
        if(!in_brackets && op_level <= 1){
          ret = i;
          op_level = 1;
        }
      }
      break;
    case TK_EQ:
    case TK_NEQ:
      {
        if(!in_brackets && op_level <= 3){
          ret = i;
          op_level = 3;
        }
      }
      break;
    case TK_AND:
      {
        if(!in_brackets && op_level <= 4){
          ret = i;
          op_level = 4;
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

uint64_t eval(int p, int q, bool *if_success) {
  if (p > q) {
    /* Bad expression */
    *if_success = false;
    return -1;  // TODO: 返回值全1代表出错, 这步可能会有隐患
  }
  else if (p == q) {
    /* Single token.
     * For now this token should be a single value.
     * Return the value of the number.
     */
    if(tokens[p].type == TK_NUM){
      return atoll(tokens[p].str);
    }
    else if(tokens[p].type == TK_HEXNUM){
      return strtoll(tokens[p].str, NULL, 16);  //! 16进制字符串转long long
    }
    else if(tokens[p].type == TK_REG){
      return isa_reg_str2val(tokens[p].str, if_success);
    }
    else{
      *if_success = false;
      return -1;
    }
  }
  else if (check_parentheses(p, q) == true) {
    /* The expression is surrounded by a matched pair of parentheses.
     * If that is the case, just throw away the parentheses.
     */
    return eval(p + 1, q - 1, if_success);
  }
  else {
    int op_index = find_main_op(p, q);
    if(op_index == -1){
      if(tokens[p].type == TK_DEREF){   //! 在这里处理解引用
        uint64_t val3 = eval(p+1, q, if_success);
        if(!if_success) return -1;      //! 访问pmem前先确认解析是否正确

        return paddr_read(val3, 8);
      }
      else{
        return -1;
      }
    }
    uint64_t val1 = eval(p, op_index - 1, if_success);
    uint64_t val2 = eval(op_index + 1, q, if_success);

    switch (tokens[op_index].type) {
      case TK_PLUS: return val1 + val2;
      case TK_MINUS: return val1 - val2;
      case TK_MULTI: return val1 * val2;
      case TK_DIV: {
        if(val2 == 0) {
          *if_success = false;
          return 0; // TODO: 除以0则代表是非法表达式, 暂时就认为结果为0
          } 
        return val1 / val2;
      }
      case TK_EQ: return val1 == val2;
      case TK_NEQ: return val1 != val2;
      case TK_AND: return val1 && val2;
      default: *if_success = false; return -1;
    }
  }
}


uint64_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // TODO: 打印所有识别的东西
  // for(int i=0; i<nr_token; i++){
  //   printf("第%d个token: %s\n", i, tokens[i].str);
  // }

  /* TODO: Implement code to evaluate the expression. */

  int i;
  for (i = 0; i < nr_token; i ++) { //! 识别解引用符号
    if (tokens[i].type == TK_MULTI && (i == 0 || tokens[i - 1].type == TK_EQ ||
    tokens[i - 1].type == TK_PLUS || tokens[i - 1].type == TK_MINUS || 
    tokens[i - 1].type == TK_AND || tokens[i - 1].type == TK_DIV ||
    tokens[i - 1].type == TK_LK || tokens[i - 1].type == TK_NEQ 
    //|| tokens[i - 1].type == TK_MULTI
    ) ) {
      tokens[i].type = TK_DEREF;
    }
  }

  return eval(0, nr_token-1, success);
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

  int total_test = 0;
  int success_test = 0;
  int failed_test = 0;
  while (!feof(fp)) 
  { 
      temp_p = fgets(StrLine,1024,fp); //读取一行
      if(temp_p == NULL){
        // printf("Error in test_expr.\n"); 
        break; 
      }

      StrLine[strlen(StrLine) - 1] = '\0'; // 必须要忽略最后的换行符

      char *result_ptr = strtok(StrLine, " ");
      char *expr_ptr = strtok(NULL, " ");
      bool if_success = true;
      uint64_t actual_result = expr(expr_ptr, &if_success); //! 由于uint64_t的特殊, 改为比较字符串
      char actual_result_ptr[50] = {0};
      sprintf(actual_result_ptr, "%lu", actual_result);
      total_test++;
      
      if(!if_success || strcmp(actual_result_ptr, result_ptr)){
        printf("Error: expected:%s,\tactual:%s,\t%s\n", result_ptr, actual_result_ptr, expr_ptr);
        failed_test++;
      }else{
        success_test++;
      }
  } 
  printf("Test expr finished. Total tests: %d, Success tests: %d, Failed tests: %d\n", total_test, success_test, failed_test);
  fclose(fp); //关闭文件
  return; 
}