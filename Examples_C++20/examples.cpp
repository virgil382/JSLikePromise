#include <iostream>  // for cout
#include <thread>    // for this_thread::sleep_for
using namespace std;

#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAll.hpp"
#include "../JSLikePromiseAny.hpp"
using namespace JSLike;

#define main main01
#include "example01.hpp"
#undef main

#define main main02
#include "example02.hpp"
#undef main

#define main main02a
#include "example02a.hpp"
#undef main

#define main main03
#include "example03.hpp"
#undef main

#define main main04
#include "example04.hpp"
#undef main

#define main main05
#include "example05.hpp"
#undef main

#define main main06
#include "example06.hpp"
#undef main

#define main main07
#include "example07.hpp"
#undef main

#define main dmain01
#include "difference01.hpp"
#undef main

//================================================================================
int main()
{
	main01();
	main02();
  main02a();
  main03();
  main04();
  main05();
  main06();
  main07();

  dmain01();
}
