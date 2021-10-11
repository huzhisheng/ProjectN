#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>
#include <sdb/watchpoint.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

static int cmd_c(char *args);
static int cmd_q(char *args);
static int cmd_help(char *args);
static int cmd_si(char *args);
static int cmd_info(char *args);
static int cmd_x(char *args);
static int cmd_w(char *args);
static int cmd_d(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  // TODO: Add more commands
  { "si", "Run single step", cmd_si },
  { "info", "Print the program state", cmd_info},
  { "x", "Print the memory data", cmd_x},
  { "w", "Set a watch point", cmd_w},
  { "d", "delete a watch point", cmd_d},
};

#define NR_CMD ARRLEN(cmd_table)

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}

// TODO: 单步执行
static int cmd_si(char *args){
  char *arg = strtok(NULL, " ");
  int i = 1;

  if (arg != NULL) {
    i = atoi(arg);  // atoi失败会返回0
  }
  if (i == 0){
    printf("Error: Please input \'si [N]\'.\n");
    return -1;
  }

  cpu_exec(i);

  return 0;
}

// TODO: 打印程序状态
static int cmd_info(char *args){
  char *arg = strtok(NULL, " ");

  if(*arg == 'r'){
    isa_reg_display();
    return 0;
  }
  else if(*arg == 'w'){
    // TODO: 打印监视点信息
    info_wp();
    return 0;
  }
  else{
    printf("Error: Please input \'info [r|w]\'.\n");
    return -1;
  }
}

// TODO: 打印内存数据
static int cmd_x(char *args){
  char *arg = strtok(NULL, " ");
  if(arg == NULL){
    printf("Error: Please input \'x [N] [paddr]\'.\n");
    return -1;
  }
  int n = atoi(arg);

  // 获取下一个参数
  arg = strtok(NULL, " ");
  if(arg == NULL){
    printf("Error: Please input \'x [N] [paddr]\'.\n");
    return -1;
  }

  paddr_t paddr;
  sscanf(arg,"%x",&paddr);
  word_t word; 
  int i;
  for(i = 0; i < n; i++){
    if(i % 4 == 0) printf("\n");
    word = paddr_read(paddr + i * sizeof(word_t), 8);
    printf(FMT_WORD" ", word);
  }
  printf("\n");

  return 0;
}

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

static int cmd_w(char *args){
  char *arg = strtok(NULL, " ");
  set_wp(arg);
  return 0;
}

static int cmd_d(char *args){
  char *arg = strtok(NULL, " ");
  int n = atoi(arg);
  del_wp(n);
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
