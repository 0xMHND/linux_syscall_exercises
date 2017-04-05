
#Find the greatest product of five consecutive digits in the 1000-digit number





def function2(i,num, mult_list):
    for i in range((len(num)-4)):
        mult_list[i] =(int(num[i])*int(num[i+1])*int(num[i+2])*int(num[i+3])*int(num[i+4]))
    return max(mult_list)

def print_Bgst(biggest):
    print("Greatest product= ", biggest)


def f():
    
    start = time.time()
    num = [line.rstrip('\n') for line in open('num.txt')]
    num = ''.join(num)
    mult_list=[0 for x in range(1000)]
    i   = 0
    
    biggest = function2(i,num,mult_list)
    print_Bgst(biggest)

    elapsed = (time.time() - start)
    print("This code took: " + str(elapsed) + " seconds")


