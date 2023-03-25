#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/vaddr.h>
enum {
  TK_NOTYPE = 256, TK_EQ,  //空格的类型
  TK_NUM,//十进制数字的类型
  TK_ADD,TK_REG,TK_HEX,TK_DEREF,TK_PLUS,TK_SUB,TK_MUL,TK_DIV,TK_OR,TK_AND,
  TK_LBR,TK_RBR,

  /* TODO: Add more token types */  //这里要添加更多的token类型

};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules. 
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},         // plus
  {"==", TK_EQ},        // equal
  {"0[Xx][0-9a-fA-F]+", TK_HEX},//十六进制数字 注意这个必须在十进制数字上面
  {"[0-9]+",TK_NUM},       //十进制数字
  {"\\(",TK_LBR},          //左括号
  {"\\)",TK_RBR},          //右括号
  {"\\-",TK_SUB},          
  {"\\*",TK_MUL},
  {"\\/",TK_DIV},
  {"\\$[a-zA-Z]+", TK_REG}, //寄存器
  {"\\|\\|", TK_OR},
  {"&&",TK_ADD},
  //还需添加十进制整数 - * / 和括号
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {  //初始化正则表达式
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
}

typedef struct token {
  int type;
  char str[32];  //记得要加\0
} Token;

static Token tokens[32] __attribute__((used)) = {};//用于按顺序存放已经被识别出的token信息 注意结构体数组长度只有32
static int nr_token __attribute__((used))  = 0;//nr_token指示已经被识别出的token数目.

static bool make_token(char *e) { //用于识别token
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
        
        if(substr_len>=32){  //这个32指的是Token结构体中存储额外信息（数字）str的数组长度
          printf("%.*s  The length of the substring is too long.\n", substr_len, substr_start);
          return false;
        }

        if(nr_token>=32){     //这个32是Token数组的长度
          printf("The count of tokens(nr_token) is out of the maximum count(32)\n");
			    return false;
        }

        switch (rules[i].token_type) { //记录在成功识别出token后, 将token的信息依次记录到tokens数组中.
          case TK_NOTYPE: //如果是空格 不用存
          break;
          case TK_NUM:
          case TK_HEX:
          case TK_REG:
                  strncpy(tokens[nr_token].str,substr_start,substr_len);
                    tokens[nr_token].str[substr_len]='\0';
          default:  tokens[nr_token].type= rules[i].token_type;
		  			++ nr_token;
                    break;
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
//-----------------------------------------------------------------------------------

int check_parentheses(int p, int q)  //检查表达式是否被一对括号包围 1->是 0->表达式合法 但未被包围 -1->非法表达式
{
	bool LR = false;
	if(tokens[p].type == TK_LBR && tokens[q].type ==TK_RBR) LR = true;
	int top = 0, i;
	for(i = p; i <= q; ++ i){ //检测括号是否成对出现
		if(tokens[i].type == TK_LBR) ++ top;
		else if(tokens[i].type == TK_RBR) -- top;
		if(top < 0) return -1;
	}
	if(top != 0) return -1;  ///排除top大于0
	if(!LR) return 0;

	/* Beware of such case like: (4 + 5) - (6 - 1)  */
	
	for(i = p + 1; i <= q - 1; ++ i){
		if(tokens[i].type == TK_LBR) ++ top;
		else if(tokens[i].type == TK_RBR) -- top;
		if(top < 0) return 0;
	}
	return 1;	// top must be zero
}

uint32_t myexit(int p, int q, bool *success){   //用于处理bad expression
	printf("Invalid expression: [%d, %d]\n", p, q);
	for(; p <= q; ++ p){
		int type = tokens[p].type;
		printf("%d: %d", p, type);
		if(type == TK_NUM || type == TK_HEX || type == TK_REG)
			printf("- %s",tokens[p].str);
		printf("\n");
	}	
	*success = false;
	return 0;
}

bool is_op(int ch){
	return ch == TK_PLUS || ch == TK_SUB || ch == TK_MUL || ch == TK_DIV
			|| ch == TK_AND || ch == TK_OR || ch == TK_EQ || ch == TK_DEREF;
}

int op_priority(int op) //运算符优先级
{
	int pri;
	switch(op){
		case TK_OR:
			pri = 0;
			break;
		case TK_AND:
			pri = 1;
			break;
		case TK_EQ:
			pri = 2;
			break;
		case TK_PLUS:
		case TK_SUB :
			pri = 3;
			break;
		case TK_MUL:
		case TK_DIV:
			pri = 4;
			break;
		case TK_DEREF:
			pri = 5;
			break;
		default:
			pri = 10;
	}
	return pri;
}

int compare(int i, int j){
	int p1 = op_priority(tokens[i].type);
	int p2 = op_priority(tokens[j].type);
	return p1 < p2 ?  -1 : (p1 == p2 ? 0 : 1);
}

int get_main_op(int p, int q)
{
	int inBracket = 0, i, pos = -1;
	for(i = p; i <= q; ++ i) {
		int type = tokens[i].type;
		if( !inBracket && is_op(type)){  //只有不在括号里的运算符 比较优先级（越小越后算） 才能找出
			if(pos == -1) pos = i;
			else if(compare(i, pos) <= 0 ) pos = i; 
		}
		else if(type == TK_LBR ) inBracket ++ ;  //在括号里的运算符一定不是主运算符
		else if(type == TK_RBR ) inBracket -- ;
	}
	return pos;
}

/* 
*   表达式求值
*/

//根据提示 要利用好expr() 中的bool指针 来判断下层递归的结果是否合法 来告诉上层递归 从而做出不同的判断
uint32_t eval(int p, int q, bool *success)
{
	if(p > q) {     // 显然是个bad expression
		return myexit(p, q, success);
	}else if(p == q){     //单个token  只能是数字  又加了是寄存器的这个可能
		uint32_t val = 0;
		int type = tokens[p].type;
		if(type == TK_NUM || type == TK_HEX) {
			return strtoul(tokens[p].str, NULL, 0);
		}
		else if(type == TK_REG) {
			val = isa_reg_str2val(tokens[p].str + 1, success); //在 reg.c 文件中
			if(*success) return val;
			printf("Unknown register: %s\n", tokens[p].str);
			return 0; 
		}
		return myexit(p, q, success);  //走到这说明上面的token都不匹配 那就是错的 调用myexit()来打印错误内容
	}

	int ret = check_parentheses(p, q);
	if(ret == -1) {
		return myexit(p, q, success);
	}
	
	if(ret == 1) {
		return eval(p + 1, q - 1, success);
	}
	
	int pos = get_main_op(p, q);  //找主运算符
	if(pos == -1){
		return myexit(p, q, success);
	}
		
	uint32_t val1 = 0, val2 = 0, val = 0;  //递归求值部分
	if(tokens[pos].type != TK_DEREF)  //主运算符可能是解引用 *rax 那么前面的表达式就算有 也没什么意义
		val1 = eval(p, pos - 1, success);

	if(*success == false) return 0;
	val2 = eval(pos + 1, q, success);
	if(*success == false) return 0;

	switch(tokens[pos].type){  //根据主运算符的类型求值 然后返回
		case TK_PLUS:
			val = val1 + val2;
			break;
		case TK_SUB:
			val = val1 - val2;
			break;
		case TK_MUL:
			val = val1 * val2;
			break;
		case TK_DIV:
			if(val2 == 0) {  //除数是否为0
				printf("Divide 0 error at [%d, %d]", p, q);
				return *success = false;
			}
			val = val1 / val2;
			break;
		case TK_AND:
			val = val1 && val2;
			break;
		case TK_OR:
			val = val1 || val2;
			break;
		case TK_EQ:
			val = val1 == val2;
			break;
		case TK_DEREF:
			val = vaddr_read(val2, 4);
			break;	
		default:
			printf("Unknown token type: %d\n", tokens[pos].type);
			return *success = false;
	}

	return val;
}



void erase(int p, int cnt){     //用来删除多余的 + -
	int i;
	for(i = p; i + cnt < nr_token; ++ i){
		tokens[i] = tokens[i + cnt];
	}
	nr_token -= cnt;
}

word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  /* TODO: Insert codes to evaluate the expression. */
  int i, type;
  for(i = 0; i < nr_token; ++ i){	 //这个循环是处理多个+-问题的 形如：+-- -- ++-等
	type = tokens[i].type; 
	if(type == TK_PLUS || type == TK_SUB)
	{
		int j = i;
		int flag = 1;
		while(j < nr_token && (type == TK_PLUS || type == TK_SUB)){
			flag *= (type == TK_PLUS ? 1 : -1);
			type = tokens[++ j].type;
		}
		if(j - i > 1){
			tokens[i].type = (flag == 1? TK_PLUS : TK_SUB) ;
			erase(i + 1, j - i - 1);
		}
	}
  } 
   
  for(i = 0; i < nr_token; ++ i){ //这个循环是处理解引用寄存器的情况 我们在make_token中只会把两个*都当作乘号
	if(tokens[i].type == TK_MUL && (i == 0 || is_op(tokens[i - 1].type))){
		if(i == 0 || tokens[i - 1].type != TK_DEREF)	// Maybe this case: **
			tokens[i].type = TK_DEREF;
	}
  }

  uint32_t ret = eval(0, nr_token - 1, success);//经过两个循环处理后 可正确调用eval函数
  return ret;
}
