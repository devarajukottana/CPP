#include<iostream>
int fun(int a, int b)
{
    a=30;
    b=40;
}
using namespace std;
int main()
{
    int x=10;
    int y=20;
    cout<<fun(x,y);
    cout<<"the x = "<<x<<" y = "<<y<<endl;

}