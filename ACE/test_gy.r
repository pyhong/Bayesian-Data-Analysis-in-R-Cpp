gy1<-read.table("gy1.txt",head=F)
gy2<-read.table("gy2.txt",head=F)
GY<-read.table("GY.txt",head=F)
gy1<-t(tail(gy1,500))
gy1_mean<-rowMeans(gy1)
gy2<-t(tail(gy2,500))
gy2_mean<-rowMeans(gy2)
plot(GY[,2],gy1_mean)
points(GY[,2],GY[,1],col="red")
plot(GY[,4],gy2_mean)
points(GY[,4],GY[,3],col="red")
