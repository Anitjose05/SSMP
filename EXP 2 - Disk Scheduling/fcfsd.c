#include<stdio.h>
int head,a[20],i,distance,n,seektime;
int main() {
 printf("Enter the head position:");
 scanf("%d",&head);
 printf("Enter the no:of disk requests:");
 scanf("%d",&n);
 printf("Enter the disk requests:");
 for(i=1;i<=n;i++) {
  scanf("%d",&a[i]);
 }
 a[0]=head;
 printf("\n\tFCFS DISK SCHEDULING\n\n");
 for(i=0;i<n;i++) {
  distance=(a[i]>a[i+1]?a[i]-a[i+1]:a[i+1]-a[i]);
  printf("Head movement from %d to %d : %d\n", a[i],a[i+1],distance);
  seektime=seektime+distance;
 }
 printf("\nTotal seek time is:%d\n",seektime);
}
