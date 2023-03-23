#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/vaddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

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
	nemu_state.state=NEMU_QUIT;	
       	return -1;
}

static int cmd_si(char *args)
{
  char *arg=strtok(NULL," ");
	int step;
	if(arg==NULL) step=1;
	else
	{
		sscanf(arg,"%d",&step);	
	}
	cpu_exec(step);
	return 0;
}

static int cmd_info(char *args)
{
  printf("%d\n",strcmp(args,"r"));
  char *arg=strtok(NULL," ");
  if(arg==NULL) 
  {
    printf("Missing extra letters\n");
    return 1;
  }
	if(strcmp(arg,"r")==0)
	{
    isa_reg_display();
	}	
  return 0;
}


static int cmd_x(char *args)
{
  char *arg=strtok(NULL," ");
  int num;
  int exp;
  if(arg==NULL)
  {
    printf(" Missing additional parameters \n");
    return 1;
  }
  sscanf(arg,"%d",&num);
  arg=strtok(NULL," ");
  if(arg==NULL)
  {
    printf(" Missing additional parameters \n");
    return 1;
  }
  sscanf(arg,"%x",&exp);
  printf("%d %x\n",num,exp);
  for(int i=0;i<num;i++)
  {
    printf("0x%08x 0x%08x\n",exp+i*4,vaddr_read(exp+i*4,4));
  }
  return 0;
}

static int cmd_p()
{
  return 0;
}

static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  /* TODO: Add more commands */
  { "q", "Exit NEMU", cmd_q },
  {"si", "single-step exeution", cmd_si },
  {"info", "Print register", cmd_info},
  {"x", "Scan Memory Print Memory Value",cmd_x},
  {"p","求出当前表达式的值",cmd_p},
};

#define NR_CMD ARRLEN(cmd_table)

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
