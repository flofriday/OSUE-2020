# 2-intmul-jozott

## Rating
**Points received:** not known 🤷‍♀️


## About this solution + bonus

Calculates large integer as hexadecimal values. Only numbers with same length can be added. It is also important that the length of the numbers are element of the second power series (1,2,4,8,16,...).

Valid inputs are:
```
./intmul
3 
3
```
```
./intmul
1A 
B3
```
```
./intmul
13A5D87E85412E5F 
7812C53B014D5DF8
```

By calling `./intmul -t`, instead of the result, the process tree is printed out.

The result looks like:
```
                 intmul(A3,B2)                 
     /          /          \          \        
intmul(A,B) intmul(A,2) intmul(3,B) intmul(3,2)
```

The program can be teste on [hexcalc.dobodox.com](https://hexcalc.dobodox.com).

[Original Repository](https://github.com/Jozott00/intmul)