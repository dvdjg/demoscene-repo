/*
 * Constructor/destructor lists (linker-generated) sorted by priority.
 *
 * Purpose: object files contribute Init/Exit functions via ADD2INIT/ADD2EXIT
 * macros; the linker merges them into __INIT_LIST__ / __EXIT_LIST__. At boot,
 * Loader calls CallFuncList(&__INIT_LIST__) before main and __EXIT_LIST__ after
 * main returns — similar in spirit to global constructors on hosted platforms.
 *
 * NOTE: possible issue: comment typo "descrutors" preserved from original.
 */
#include <system/autoinit.h>
 
/* Introduce weak symbols in case no constructors or descrutors were defined. */
FuncItemSetT __INIT_LIST__;
FuncItemSetT __EXIT_LIST__;

void CallFuncList(FuncItemSetT *set) {
  short n = set->count / 2;
  FuncItemT *start = (FuncItemT *)set->item;
  u_char cur_pri = 0;

  while (n > 0) {
    FuncItemT *item;
    u_char next_pri = 255;
    for (item = start; item->func; item++) {
      u_char pri = item->pri;
      if (pri < cur_pri)
        continue;
      if (pri == cur_pri) {
        item->func();
        n--;
      } else if (pri < next_pri) {
        next_pri = pri;
      }
    }
    cur_pri = next_pri;
  }
}
