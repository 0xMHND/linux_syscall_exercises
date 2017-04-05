



def f():
    with open('names.txt') as f:
        names = f.read().split(',')
        names.sort()

    NameValue=0
    NameScore=0
    NamesSum=0
    ScoreList=[]

    for index,i in enumerate(names):
        for j in i:
            if j!='"':
                num = ord(j)-64
                NameValue += num
        NameScore = NameValue*(index+1)
        ScoreList.append(NameScore)

        NameValue=0
        NameScore=0


    NamesSum = sum(ScoreList)
    print("Name Score result: ",NamesSum)

