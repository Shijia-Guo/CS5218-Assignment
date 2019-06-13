int main(){
    int a = -2,b = 5,x = 0, y;
    int N = 100;
    int i = 0;
    while(i++ < N){
        if(a > 0){
            x = x + 7;
            y = 5;
        }else{
            x = x -2;
            y = 1;
        }
        if(b > 0){
            a = 6;
        }else{
            a = -5;
        }
    }
}