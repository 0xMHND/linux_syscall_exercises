import SortNames_AddScores
import GreatestProd
import OS_expermints
import sys
def chose():
    print("Enter which program to run:\nSortNamesThenAddScore '1'\nBiggestFiveConsecutive '2'\nLongestPathWay '3'\n")
    return input("I chooes ")

def function(arg):
    if(arg == '1'):
        SortNames_AddScores.f()
    if(arg == '2'):
        GreatestProd.f()
    if(arg == '3'):
        OS_expermints.f()

def main():
    if len(sys.argv)==1:
        arg=chose()
    else:
        arg=sys.argv[1]
        
    function(arg)
    

if __name__ == "__main__":
    main()
