#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <assert.h>
//method to insert new account and sort by id and return message to log file
char* new(char* id, char* password, char* amount, char* atmId);

//deposit chash into account and return message to log file
char* deposit(char* id, char* password, char* amount , char* atmId);

//withdraw from account if exist and return message to log file
char* withdraw(char* id, char* password, char* amount , char* atmId);

//check the account balance and return message to log file
char* balance(char* id, char* password, char* atmId);

//close aacount and return message to the log file
char* close_acc(char* id, char* password, char* atmId);

//transfer money from account to another and return message to the log file
char* transfer(char* fromId, char* password, char* toId, char* amount, char* atmId);

//we called this method from the mian by creating thread for every atm send the number of the atm with the thread
void* Atm_thread(void * num);

//call this message if there is transaction to do. 
void atm_request(char* text, char* atmId);

//print the bank status evey 3 seconds
void print_bank();

//just for reverse the string if must
char * strrev(char *str);

//check if the account is exist
struct account* find_id(char* account);
#define BUF_SIZE 8192

struct account *head;// this is the head of the account structs
int bank_log;//we write the bank log for every transaction here
int count;//just for know if there is more transcations to do
int flag=0;//if the id exist them use semfur
sem_t mutex;//our mutex

//account struct
struct account{
    int id;
	char password[5];
    int balance;
    struct account* next;
};

//main
int main(int argc, char* argv[]) {
	int n, i, ans, command; 
	int *ATMs, *arr;
	pthread_t bank_thread, *atm_threads;
	char buffer[20];
	char file[40];
	
	head=(struct account*) malloc(sizeof(struct account));
	printf ("Enter the number of ATMs: \n");
	scanf("%d",&n);

	ATMs=(int*)malloc(sizeof(int)*n);
	atm_threads=(pthread_t*)malloc(sizeof(pthread_t)*n);
	count=n;

	sem_init(&mutex, 0, 1);
	if(argc!=n+1){
		strcpy(file,"\nPlease make sure you have");
		sprintf(buffer, " %d", n-1);
		strcat(file,buffer);
		strcat(file," ATMs files and typed it currectly");
		printf("%s\n\n",file);
		exit(0);
	}
	//create log file for the transaction
	bank_log = open("log.txt", O_WRONLY | O_CREAT, 0644);
	if(bank_log == -1){
		perror("faild open log");
		exit(1000);
	}
	
	for(i=0; i<n ; i++){
		ATMs[i]=i+1;
		ans = pthread_create(&atm_threads[i], NULL, Atm_thread, (void*)&ATMs[i]);
		if(ans != 0){
			printf("error to create thread %d \n", i);
			exit(i);		
		}
	}
	//print util there is thread running
	while(count>0){
		print_bank();
		sleep(2);
	}	
	sleep(5);
	for(i=0; i<n ; i++)	
		pthread_join ( atm_threads[i] , NULL ); 
	
	
	return 0;
}

void* Atm_thread(void * num){
	int atm = (*(int*)num);
	int input_fd, j;
	char buffer[BUF_SIZE], atm_num[3];
	char transaction[20];
	char file[40];
	int i, t, k;
	ssize_t ret_in;

	strcpy(file,"ATM_");
	sprintf(atm_num, "%d", atm);
	strcat(file,atm_num);
	strcat(file,"_input_file.txt");
	input_fd = open (file, O_RDONLY);
	if (input_fd == -1) {
		strcpy(buffer,file);
		strcat(buffer, " not found\n");
		perror (buffer);
		exit(atm);
	}
//open the atm file
	if((ret_in=read (input_fd, buffer, BUF_SIZE))<0){
		strcat(buffer, " Faild to open ");
		strcpy(buffer,file);
		perror (buffer);
		exit(atm);
	}
	i=0;
	strcpy(transaction, "");
	while(buffer[i] != '\0'){
		while(buffer[i] != '\n'){
			strncat(transaction, (buffer + i), 1);
			i++;
		}
		//sent the transaction
		atm_request(transaction, atm_num);
		usleep(100);
		strcpy(transaction, "");
		if(buffer[i] == '\0'){
}
		i++;
	}
	count--;
	close(input_fd);
}

void atm_request(char* text, char* atmId){
	struct account* temp;
	char* account=(char*)malloc(sizeof(char)*6);
	char* password=(char*)malloc(sizeof(char)*5);
	char* target=(char*)malloc(sizeof(char)*6);
	char* balane=(char*)malloc(sizeof(char)*5);
	char* amount=(char*)malloc(sizeof(char)*5);
	char* log;
	int i=2, j=0, w;


	//get the account id
	while(i<8){
		account[j]=text[i];
		i++;
		j++;
	}
	j=0;
	//get the password
	while(i<12){
		password[j]=text[i];
		i++;
		j++;
	}

	i++;
	j=0;
	//we need the amount
	if(text[0]=='O'||text[0]=='D'||text[0]=='W')
		while(text[i]!='\n'){
			amount[j]=text[i];
			i++;
			j++;
		}
	i++;
	temp=find_id(account);

//casese for every 6 methods new, deposit, withdraw, balance, transfer, remove
	switch(text[0]){

		case 'O':
			log=new(account, password, amount, atmId);	
			sleep(1);	
			break;

		case 'D':
			if(flag==1)
				sem_wait(&mutex);

			log=deposit(account, password, amount, atmId);	
			sleep(1);	

			if(flag==1)
				sem_post(&mutex);
			flag=0;
			break;

		case 'W':
			if(flag==1)
				sem_wait(&mutex);

			log=withdraw(account, password, amount, atmId);
			sleep(1);

			if(flag==1)
				sem_post(&mutex);
			flag=0;		
			break;

		case 'B':
			if(flag==1)
				sem_wait(&mutex);

			log=balance(account, password, atmId);
			sleep(1);

			if(flag==1)
				sem_post(&mutex);
			flag=0;		
			break;	
	
		case 'Q':
			log=close_acc(account, password, atmId);
			sleep(1);		
			break;

		case 'T':
			if(flag==1)
				sem_wait(&mutex);

			j=0;
			while(i<19){
				target[j]=text[i-1];
				i++;
				j++;
			}
			j=0;
			while(text[i]!='\0'){
				amount[j]=text[i];
				i++;
				j++;
			}
		 	log=transfer(account, password, target, amount, atmId);
			sleep(1);

			if(flag==1)
				sem_post(&mutex);
			flag=0;			
			break;
			
		default:
			break;


	}
	//write to the log file the transaction
	strcat(log, "\n\n");
	if((w = write (bank_log, log, strlen(log)))<0){
		perror("faild to write to the log file\n");
		exit(4);
	}

	
}

char* new(char* id, char* password, char* amount, char* atmId){
	struct account *temp=(struct account*) malloc(sizeof(struct account));
	struct account *temp2=(struct account*) malloc(sizeof(struct account));
	struct account	*prev=(struct account*) malloc(sizeof(struct account));
	struct account	*curr=(struct account*) malloc(sizeof(struct account));
	int ID, flag=0;
	char atm_num[3];
	int num = strtol(id, NULL, 10);	
	char* log = (char*)malloc(sizeof(char*)*400);

//there is no accounst
	if(head->id==0){
		head->id=num;
		strcpy(head->password, password);
		head->balance=atoi(amount);
//	printf("%d %s %d \n",head->balance, amount, atoi(amount));
		head->next=NULL;
		flag=1;
	}
//the account already exist
	else if((head->id)==num){
		strcpy(log, "Error "); 
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account with the same Id exists");
	}
	//here we have to check where to insert the account sorted by id
	else if((head->id)>num){
		temp->id=num;
		
		strcpy(temp->password,password);
		num = strtol(amount, NULL, 10);	
		temp->balance=num;
		temp->next=head;
    	head=temp;
		flag=1;		
	}

	else{
		temp=head;
		while(num>(temp->id)&&temp->next!=NULL){
				prev=temp;
				temp=temp->next;
		}
		//this the last node;
		if(prev->next==NULL){
			num = strtol(id, NULL, 10);	
			temp2->id=num;
			strcpy((temp2->password),password);
			num = strtol(amount, NULL, 10);	
			temp2->balance=num;	
			temp2->next=NULL;
			temp->next=temp2;	
		}

		//there is more nodes;
		else{
			num = strtol(id, NULL, 10);	
			curr->id=num;
			strcpy((curr->password),password);
			num = strtol(amount, NULL, 10);	
			curr->balance=num;
			curr->next=temp;
			prev->next=curr;
			
		}	
		flag=1;						
	}//to the log file
		if(flag==1){
			strcpy(log, atmId); 
			strcat(log, ": New account id is ");	
			strcat(log, id);	
			strcat(log, " with password ");
			strcat(log, password);
			strcat(log, " and initial balance ");
			strcat(log, amount);			
		}
return log;

}

char* deposit(char* id, char* password, char* amount , char* atmId){
	struct account* temp=head;
	int ID, cmp;
	char* log = (char*)malloc(sizeof(char*)*400);
	char id_str[6];
	char str[10];
	int num = strtol(id, NULL, 10);	
	while(temp->id<num&&temp->next!=NULL)
		temp=temp->next;
	cmp=strcmp(temp->password,password);
//the account id is incorrect
	if (temp->id==num&&cmp!=0){
		strcpy(log, "Error "); 
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - password for account id ");
		sprintf(str, "%d", temp->id);
		strcat(log, str);	
		strcat(log, " is incorrect");
	}
//the account is correct
	else if(temp->id==num&&cmp==0){
		num = strtol(amount, NULL, 10);
		temp->balance+=num;
		strcpy(log, atmId); 
		strcat(log, ": Account ");
		sprintf(str, "%d", temp->id);
		strcat(log, str);	
		strcat(log, " now balance is ");
		sprintf(str, "%d", temp->balance);					
		strcat(log, str);	
		strcat(log, " after ");
		strcat(log, amount);
		strcat(log, " $ was deposited");
	}
	
	else{
		strcpy(log, "Error ");
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account id ");
		strcat(log, id);
		strcat(log, " does not exits");		
	}

	return log;
}

char* withdraw(char* id, char* password, char* amount , char* atmId){
	struct account* temp=head;
	int ID, cmp;
	char* log = (char*)malloc(sizeof(char*)*400);
	char str[10];
	int num = strtol(amount, NULL, 10);
//printf("%d %s\n",atoi(id),password);
	while(temp->id<=atoi(id)&&temp->next!=NULL)
		temp=temp->next;
//printf("%d %s\n",temp->password,password);
	cmp=strcmp(temp->password,password);
	if (temp->id==atoi(id)&&cmp!=0){
		strcpy(log, "Error "); 
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - password for account id ");
		sprintf(str, "%d", temp->id);					
		strcat(log, str);
		strcat(log, " is incorrect");
	}

	else if(temp->id==atoi(id)&&cmp==0){
		if(num>temp->balance){
			strcpy(log, "Error "); 
			strcat(log, atmId);
			strcat(log, ": Your transaction faild - account id ");
			sprintf(str, "%d", temp->id);					
			strcat(log, str);
			strcat(log, " balance is lower than ");
			strcat(log, amount);
		//printf("%s\n",log);	
		}

		else{
			temp->balance-=num;
			strcpy(log, atmId); 
			strcat(log, ": Account ");
			sprintf(str, "%d", temp->id);								
			strcat(log, str);
			strcat(log, " new balance is ");
			sprintf(str, "%d", temp->balance);					
			strcat(log, str);			
			strcat(log, " after ");
			strcat(log, amount);
			strcat(log, " $ was withdrew");
		}
	}

	else{
		strcpy(log, "Error ");
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account id ");
		strcat(log, id);
		strcat(log, " does not exits");		
	}
	return log;
}
//check the balnace
char* balance(char* id, char* password, char* atmId){
	struct account* temp=head;
	int ID, cmp;
	char* log = (char*)malloc(sizeof(char*)*400);
	char str[10];
	int num = strtol(id, NULL, 10);
//search for the account
	while(temp->id<num)
		if(temp->next!=NULL)
			temp=temp->next;

	cmp=strcmp(temp->password,password);
//password incorrect
	if (temp->id==num&&cmp!=0){
		strcpy(log, "Error "); 
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - password for account id ");
		sprintf(str, "%d", temp->id);
		strcat(log, str);
		strcat(log, " is incorrect");
	}
//return the balance
	else if(temp->id==num&&cmp==0){
			strcpy(log, atmId); 
			strcat(log, ": Account ");
			sprintf(str, "%d", temp->id);
			strcat(log, str);
			strcat(log, " balance is ");
			sprintf(str, "%d", temp->balance);
			strcat(log, str);			

	}
//there is no account with this id
	else{
		strcpy(log, "Error ");
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account id ");
		strcat(log, id);
		strcat(log, " does not exits");		
	}

	return log;
}
//remove the account
char* close_acc(char* id, char* password, char* atmId){
	struct account* temp=NULL;
	struct account* next=NULL;
	struct account* prev=NULL;
	int non=0, cmp, flag=0, balance;
	char* log = (char*)malloc(sizeof(char*)*400);
	char str[10];
	temp=head;
	int num = strtol(id, NULL, 10);
	if(head->id==num && (cmp=strcmp(head->password,password))==0){
			balance=head->balance;
			if(head->next==NULL){
				head->id=0;
			}
			else
				head=head->next;	
			flag=1;
			non=1;
	}
	else{
		while(temp->id<num&&temp->next!=NULL){
				prev=temp;
				temp=temp->next;
			}
		cmp=strcmp(temp->password,password);

		if (temp->id==num&&cmp!=0){
			strcpy(log, "Error "); 
			strcat(log, atmId);
			strcat(log, ": Your transaction faild - password for account id ");
			sprintf(str, "%d", temp->id);
			strcat(log, str);	
			strcat(log, " is incorrect");
			non=1;
		}
		else if(temp->id==num&&cmp==0){
			flag=1;
			non=1;
			balance=temp->balance;

			//we have to remove node from the middle
			if(temp->next!=NULL){
				prev->next=temp->next;
				temp=NULL;
			}
			//this is the last node
			else{
				prev->next=NULL;
			}
		}
	}

	if(non==0){
		strcpy(log, "Error ");
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account id ");
		strcat(log, id);
		strcat(log, " does not exits");		
	}

	if(flag==1){
		strcpy(log, atmId); 
		strcat(log, ": Account ");
		strcat(log, id);	
		strcat(log, " is now closed. balance was ");	
		sprintf(str, "%d", balance);
		strcat(log, str);			
	}

	return log;
}
//transfer
char* transfer(char* fromId, char* password, char* toId, char* amount, char* atmId){
	struct account* from=head;
	struct account* to=head;
	int ID, cmp, flag=0;
	char* log = (char*)malloc(sizeof(char*)*400);
	char str[10];
	int num = strtol(fromId, NULL, 10);
//search the account
	while(from->id<num&&from->next!=NULL)
			from=from->next;
//password incorrect
	cmp=strcmp(strrev(from->password),password);
	if (from->id==num&&cmp!=0){
		strcpy(log, "Error "); 
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - password for account id ");
		sprintf(str, "%d", from->id);
		strcat(log, str);		
		strcat(log, " is incorrect");
	}
//the account from exist. let find to where to transfer	
	else if(from->id==num&&cmp==0){
		flag++;
		num = strtol(strrev(toId), NULL, 10);
		while(to->id<num&&to->next!=NULL)
			to=to->next;

		
		if (to->id==num)
			flag++;
	}
	if(flag==0){
		strcpy(log, "Error ");
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account id ");
		strcat(log, fromId);
		strcat(log, " does not exits");		
	}
	else if(flag==1){
		strcpy(log, "Error ");
		strcat(log, atmId);
		strcat(log, ": Your transaction faild - account id ");
		strcat(log, toId);
		strcat(log, " does not exits");	
	}
	else if(flag==2){
		num = strtol(amount, NULL, 10);	
		if(from->balance<num){
			strcpy(log, "Error "); 
			strcat(log, atmId);
			strcat(log, ": Your transaction faild - account id ");
			sprintf(str, "%d", from->id);
			strcat(log, str);
			strcat(log, " balance is lower than ");
			strcat(log, amount);	
		}
		else{
			num=atoi(amount);
			from->balance-=num;
			to->balance+=num;
			strcpy(log, atmId); 
			strcat(log, ": Transfer ");
			strcat(log, amount);	
			strcat(log, " from account ");	
			sprintf(str, "%d", from->id);		
			strcat(log, str);
			strcat(log, " to account ");
			sprintf(str, "%d", to->id);
			strcat(log, str);	
			strcat(log, " new account balance is");		
			sprintf(str, "%d", from->balance);
			strcat(log, str);	
			strcat(log, " new target account balance is");	
			sprintf(str, " %d", to->balance);
			strcat(log, str);	
		}
	}
	return log;
}
//print the bank status every 3 seconds
void print_bank(){
	struct account* temp=head;
	int balance=0;



	printf("Current bank Status\n");

	while(temp!=NULL&&temp->balance!=0){
		
		balance+=temp->balance;
		printf("Account %d: balance = %d $ , Account Password = %s\n",temp->id, temp->balance, temp->password);
		temp=temp->next;		
	}
	
	printf("The Bank has %d\n\n",balance);

}
	


char * strrev(char *str){
    int i = strlen(str)-1,j=0;
    char ch;
    while(i>j){
        ch = str[i];
        str[i]= str[j];
        str[j] = ch;
        i--;
        j++;
    }
    return str;
}
//search for the account and return it
struct account* find_id(char* account){
	struct account *temp=(struct account*) malloc(sizeof(struct account));
	temp=head;
	if(temp->id!=0){
		while (temp->id!=strtol(account, NULL, 10)&&temp->next!=NULL)
			temp=temp->next;
		if(temp->id==strtol(account, NULL, 10))
			flag=1;
		}
	return temp;
}	
