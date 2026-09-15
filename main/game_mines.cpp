#include "game_mines.h"
#include <algorithm>
#include <cstdlib>
namespace gameMines {
uint32_t Game::random(){rng_^=rng_<<13;rng_^=rng_>>17;rng_^=rng_<<5;return rng_;}
void Game::plant(unsigned safe){
 unsigned choices[36],n=0;
 for(unsigned i=0;i<36;++i)if(std::abs((int)(i%6)-(int)(safe%6))>1||std::abs((int)(i/6)-(int)(safe/6))>1)choices[n++]=i;
 for(unsigned i=0;i<6;++i){unsigned j=i+random()%(n-i);std::swap(choices[i],choices[j]);s_.cells[choices[i]].mine=true;}
 for(int i=0;i<36;++i)for(int y=-1;y<=1;++y)for(int x=-1;x<=1;++x){int r=i/6+y,c=i%6+x;if(r>=0&&r<6&&c>=0&&c<6&&s_.cells[r*6+c].mine)++s_.cells[i].adjacent;}
 s_.phase=Phase::Playing;
}
bool Game::flag(unsigned i){
 if(i>=36||s_.cells[i].revealed||s_.phase==Phase::Won||s_.phase==Phase::Dead)return false;
 auto&c=s_.cells[i];if(!c.flag&&s_.flags>=6)return false;c.flag=!c.flag;if(c.flag)++s_.flags;else --s_.flags;return true;
}
bool Game::reveal(unsigned i){
 if(i>=36||s_.cells[i].revealed||s_.cells[i].flag||s_.phase==Phase::Won||s_.phase==Phase::Dead)return false;
 if(s_.phase==Phase::Ready)plant(i);
 if(s_.cells[i].mine){s_.cells[i].revealed=true;s_.phase=Phase::Dead;return true;}
 unsigned queue[36],head=0,tail=0;queue[tail++]=i;s_.cells[i].revealed=true;++s_.opened;
 while(head<tail){unsigned pos=queue[head++];if(s_.cells[pos].adjacent)continue;
  for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx){int r=(int)(pos/6)+dy,c=(int)(pos%6)+dx;if(r<0||r>=6||c<0||c>=6)continue;
   auto&next=s_.cells[r*6+c];if(!next.mine&&!next.flag&&!next.revealed){next.revealed=true;++s_.opened;queue[tail++]=r*6+c;}
  }
 }
 if(s_.opened==30) s_.phase=Phase::Won;
 return true;
}
}
