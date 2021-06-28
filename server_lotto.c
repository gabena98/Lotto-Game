#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/sem.h>
#include <malloc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <unistd.h>

#define BUF_SIZE 1024
#define OP_CODE_SIZE 20
#define true 1
#define false 0

typedef int bool;

int fattoriale(int k){//k quantita' numeri della combinazioine
    if(k==1)
        return 1;
    return k*fattoriale(k-1);
}

int combinazioni(int n,int diff){//n quantita' numeri della serie, diff =n-k+1
    if(n==diff)
        return n;
    return n*combinazioni(n-1,diff);
}

int combinazionisenzaripetizione(int n, int k){
    return combinazioni(n, n-k+1)/fattoriale(k);
}

char *strptime(const char *buf, const char *format, struct tm *tm);

int main(int argc, char* argv[]) {
   
     if((argc>3)|| (argc==1) ){
        printf("Paramentri per la configurazione del server non validi \n");
        return(0);
    }
     
    int port,periodo,len, ret, sd,new_sd;
    struct sockaddr_in my_addr, cl_addr;
    uint16_t dim;
    pid_t pid,pid_estrazione;
    char op_code[OP_CODE_SIZE];//nome operazione inviata dal client
    char stringa_periodo[100];// periodo dato in ingresso
    char arg[BUF_SIZE];
    char risposta[BUF_SIZE];
    char session_id[11];
    char ip[20];
    char username_client[BUF_SIZE];
    time_t* ultima_estrazione;//puntatore a memoria condivisa
    sem_t* mutex,*sem;//puntatore a sem condiviso
    int *n_estrazioni;//contiene il numero di estrazioni 
    int shmidtime,shmidsem,shmidmutex,shmidint;//identificatore memoria condivisa
    
    
    port=atoi(argv[1]);
    
    if(!(argv[2])){
        periodo=300;//default 5 minuti
    }
    else{
        //formato di ingresso H:M:S
        int h,m,s;
        strcpy(stringa_periodo,argv[2]);
        sscanf(stringa_periodo,"%d:%d:%d",&h,&m,&s);
        periodo=s+(m*60)+(h*60*60);
    }
    //creo segmento condiviso per variabile time_t
    shmidtime=shmget(IPC_PRIVATE,sizeof(time_t),0600);
    if(shmidtime<0){
        printf("shmget error\n");
        exit(1);
    }
    ultima_estrazione=(time_t*)shmat(shmidtime,0,0);
    //inizializzo semaforo mutex in memoria condivisa
    shmidmutex=shmget(IPC_PRIVATE,sizeof(sem_t),0600);
    if(shmidmutex<0){
        printf("shmget error\n");
        exit(1);
    }
    mutex=(sem_t*)shmat(shmidmutex,0,0);
    sem_init(mutex,1,1);
    //inizializzo semaforo sem in memoria condivisa
    shmidsem=shmget(IPC_PRIVATE,sizeof(sem_t),0600);
    if(shmidsem<0){
        printf("shmget error\n");
        exit(1);
    }
    sem=(sem_t*)shmat(shmidsem,0,0);
    sem_init(sem,1,1);
    //inizializzo segmento condiviso per variabile int
    shmidint=shmget(IPC_PRIVATE,sizeof(int),0600);
    if(shmidint<0){
        printf("shget erroe\n");
        exit(1);
    }
    n_estrazioni=(int*)shmat(shmidint,0,0);
    *n_estrazioni=0;
   
    memset(session_id,0,sizeof(session_id));
    pid_estrazione=fork();
    
    if(pid_estrazione==-1){
                printf("errore in fase di fork");
                exit(1);
    }
        //siamo nel figlio
    if(pid_estrazione==0){
        int estratti [5];
        bool doppione;
        char leggi[BUF_SIZE];
        int x,y;
        FILE* pun;
        time_t rawtime;
        struct tm *info;
        char num_ruota[50];
        //inizializzo ultima estrazione
        srand (time(0));
        time( &rawtime );
        info = localtime( &rawtime );
        sem_wait(mutex);
        *ultima_estrazione=mktime(info);
        sem_post(mutex);
        //creo file numero_estrazioni
        pun=fopen("numero_estrazioni.txt","a+");
        fseek(pun,0,SEEK_SET);//mi posiziono a inizio file
        if(fgets(leggi,10,pun)==NULL)
            fprintf(pun,"%d",0);
        else{
            sem_wait(sem);
            *n_estrazioni=atoi(leggi);
            sem_post(sem);
        }
        fclose(pun);
        while(1){
            sleep(periodo);
            srand (time(0));
            time( &rawtime );
            info = localtime( &rawtime );
            sem_wait(mutex);
            *ultima_estrazione=mktime(info);
            sem_post(mutex);
            sem_wait(sem);
            *(n_estrazioni)=*(n_estrazioni)+1;   
            pun=fopen("numero_estrazioni.txt","w");
            fprintf(pun,"%d",*n_estrazioni);
            fclose(pun);
            sem_post(sem);        
            //genero numeri estrazione lotto
            for(x=0;x<11; x++){
                sprintf(num_ruota,"ruota%d",x);
               //scrivo data estrazione
                pun=fopen(num_ruota,"a");
                fprintf(pun,"estrazione del:%s", asctime(info));
                //genero i numeri
                for(y=0;y<5;y++){
                    doppione=false;
                    estratti[y]=rand()%90+1;
                    int j;
                    for(j=0;j<y;j++){//controllo doppioni
                        if(estratti[j]==estratti[y]){
                            y--;
                            doppione=true;
                            break;
                        }
                    }
                    if(doppione)
                        continue;
                    if(y==4)
                        fprintf(pun,"%d\n",estratti[y]);
                    else
                       fprintf(pun,"%d ",estratti[y]); 
                }
                fclose(pun);
            }
            // printf("%s\n",ctime(ultima_estrazione));
        }
    }
    
    printf("\nAvvio server\nporta numero: %d\n",port);
    printf("\nCreazione socket di ascolto TCP in corso . . . \n");
    sd=socket(AF_INET,SOCK_STREAM,0);
    //creazione indirizzo di bind
    memset(&my_addr, 0, sizeof(my_addr)); // Pulizia 
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    
    ret=bind(sd,(struct sockaddr*)&my_addr, sizeof(my_addr));
    
    if(ret<0){
        printf("errore nella binding \n");
        perror("Error: \n");
        exit(1);
        
    }
    
    printf("Creazione socket completata \n");
    ret=listen(sd,10);
      
    while(1){
        printf("Attendo richieste di connessione\n");
        len=sizeof(cl_addr);
        //accetto nuove connessioni
        new_sd=accept(sd,(struct sockaddr*)&cl_addr,(socklen_t*)&len);
        inet_ntop(AF_INET,&cl_addr.sin_addr.s_addr,ip,sizeof(ip));
  
        printf("connesso con il client: %s \n",ip);
        pid=fork();
       
        if(pid==-1){
                printf("errore in fase di fork");
                close(new_sd);
                exit(1);
        }
        //siamo nel figlio
        if(pid==0){
            close(sd);
            while(1){
                printf("aspetto il comando da eseguire per il client:%s \n",ip);
                //ricevo il comando dal client
                memset(op_code,0,sizeof(op_code));
                ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                len=ntohs(dim);
                ret=recv(new_sd,(void*)op_code,len,0);
                if(ret<0){
                    printf("errore in fase di ricezione: \n");
                    continue;
                }
                
                if(!strcmp(op_code,"!esci")){
                    memset(session_id,0,sizeof(session_id));//cancello session_id
                    memset(risposta,0,sizeof(risposta));
                    strcpy(risposta,"disconnessione effettuata");
                    len=strlen(risposta)+1;
                    dim=htons(len);
                    ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    ret=send(new_sd,(void*)risposta,len,0);
                    if(ret<0){
                        printf("Impossibile inviare risposta\n");
                        close(new_sd);
                        exit(1);
                        }
                    printf("client:%s disconnesso\n",ip);
                    break;
                }
                
                if(!strcmp(op_code,"!signup")){
                    char string [BUF_SIZE];   
                    char * usrpwd;
                    char *username;
                    char *password;
                    //ricevo le credenziali
             label1:ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)arg,len,0);
                  
                    //printf("%s\n",arg);
                    len =strlen(arg);
                    //printf("%d\n",len);
                    if(ret<0){
                        printf("errore in fase di ricezione: \n");
                        break;
                    } 
                    //ottengo username
                    usrpwd=strtok(arg," ");
                    username=usrpwd;
                    //printf("%s\n",username);
                    len=strlen(username);
                    //printf("%d",len);
                    //ottengo password;
                    usrpwd=strtok(NULL,"\0,\n");
                    password=usrpwd;
                  // len=strlen(password);
                    //printf("%d",len); 
                    //printf("%s\n",password);
                    FILE *fptr= fopen("username.txt","a");
                    if (fptr  == NULL){
                        printf("Error! opening file\n");
                        // Program exits if the file pointer returns NULL.
                        exit(1);
                    }
                    fptr= fopen("username.txt","r");
                    //printf("cerco nelfile username\n");
                    char concat [BUF_SIZE];//concateno "username:" a usr
                    strcpy(concat,"username:");
                    strcat(concat,username);
                    strcat(concat," ");
                    //printf("%s\n",concat);
                    while(fgets(string,BUF_SIZE,fptr)){
                       // printf("%s",string);
                        if(strstr(string,concat)){//username non disponibile
                            strcpy(risposta, "negativo");
                            len=strlen(risposta)+1;
                            dim=htons(len);
                            ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                            ret=send(new_sd,(void*)risposta,len,0);
                            fclose(fptr);
                            memset(risposta,0,sizeof(risposta));//pulisco per i nuovi dati
                            goto label1;
                        }
                    }
                    fclose(fptr);
                    //printf("apro username in append\n");
                    fptr=fopen("username.txt","a");//in questo file salvo tutti gli account
                    fprintf(fptr,"username:%s ",username);
                    fprintf(fptr,"password:%s\n",password);
                    fclose(fptr);
                    // printf("creo client usr in append\n");
                    fptr=fopen(username,"a");//ogni utente ha un file identificato dal suo username dove salvo le schedine, lo creo ora
                    if (fptr  == NULL)
                         printf("Error! opening file\n");
                    fclose(fptr);
                    //printf("chiudo file\n");
                    strcpy(risposta,"Registrazione effettuata");
                    len=strlen(risposta)+1;
                    dim=htons(len);
                    ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    ret=send(new_sd,(void*)risposta,len,0);
                    printf("registrazione andata a buon fine\n");
                }
                
                if(!strcmp(op_code,"!login")){
                    char string [BUF_SIZE];
                    char controllo[BUF_SIZE];
                    char  s_time[50];
                    char * usrpwd;
                    char *username;
                    char *password;
                    char *pun;
                    bool trovato=false;
                    int tentativi=0;
                    FILE *fptr;
                    struct tm time_tentativi;//data che leggo nel file tentativi.txt
                    time_t t_time;
                    time_t rawtime;
                    struct tm *info;
                    //ottengo la stringa con usr pwd
                    
             label3:if(tentativi==3){
                        FILE *fileptr= fopen("tentativi.txt","a");
                        time( &rawtime );
                        info = localtime( &rawtime );
                        fprintf(fileptr,"%s-",ip);//salvo ip e timestamp del client che superato numero tentativi 3
                        fprintf(fileptr,"%s", asctime(info));
                        fclose(fileptr);
                        printf("chiudo connessione con:%s",ip);
                        close(new_sd);
                        exit(1);
                    }
                    memset(arg,0,sizeof(arg));
                    //ottengo la stringa con usr pwd
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)arg,len,0); 
                    //printf("%s\n",arg);
                    len =strlen(arg);
                    //printf("%d\n",len);
                    if(ret<0){
                        printf("errore in fase di ricezione: \n");
                        break;
                    } 
                    //creo tentativi.txt
                    fptr=fopen("tentativi.txt","a");
                    fclose(fptr);
                     //controllo se è nella lista degli ip bloccati
                    fptr=fopen("tentativi.txt","r");
                    
                    if(tentativi==0){
                        while(fgets(controllo,BUF_SIZE,fptr)){
                            pun=strtok(controllo,"-");//prendo ip  
                            if(!strcmp(pun,ip)){
                                pun=strtok(NULL,"-");//prendo il tempo del terzo tentativo fallito
                                strcpy(s_time,pun);
                            }
                        }
                    memset(controllo,0,sizeof(controllo));
                    fclose(fptr);
                    if(s_time!=NULL){
                            time( &rawtime );
                            info = localtime( &rawtime );
                            rawtime=mktime(info);//ottengo tempo attuale
                            strptime(s_time,"%c", &time_tentativi); 
                            t_time=mktime(&time_tentativi);  //ottengo tempo ultimo tentativo non andato a buon fine
                            if(difftime(rawtime,t_time)<1800){//provo a ricollegarmi nell'ultima mezzora
                                strcpy(risposta, "ip bloccato");
                                len=strlen(risposta)+1;
                                dim=htons(len);
                                ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                                ret=send(new_sd,(void*)risposta,len,0);
                                if(ret<0){
                                    perror("errore in fase di invio: \n"); 
                                    break;
                                } 
                                close(new_sd);
                                exit(1);
                            }
                        }
                    }
                    //creo username.txt nel caso in cui non si sia fatta la signup
                    fptr=fopen("username.txt","a");
                    fclose(fptr);
                     //ottengo username
                    usrpwd=strtok(arg," ");
                    username=usrpwd;
                    //printf("%s\n",username);
                    //ottengo password
                    usrpwd=strtok(NULL,",");
                    password=usrpwd;
                    //printf("%s\n",password);  
                    strcpy(string,"username:");//creo stringa username: password:\n per il controllo
                    strcat(string,username);
                    strcat(string," password:");
                    strcat(string,password);
                    strcat(string,"\n");
                   // printf("%s",string);
                    fptr= fopen("username.txt","r"); 
                    if (fptr == NULL){
                        printf("Error! opening file\n");
                        // Program exits if the file pointer returns NULL.
                        exit(1);
                    }
                    while(fgets(controllo,BUF_SIZE,fptr)){
                       // printf("%s\n",controllo);
                        if(!strcmp(string,controllo)){
                            trovato =true;
                            //genero session id
                            int i;
                            srand (time(0));
                            for (i=0; i<10; i++)
                                session_id[i] = (char) ((rand() % 26)+ 65);
                            session_id[10]='\0';
                            printf("%s\n",session_id);
                            //invio risposta
                            strcpy(risposta, "Autentificazione avvenuta con successo!");
                            printf("autentificazione avvenuta con successo\n");
                            len=strlen(risposta)+1;
                            dim=htons(len);
                            ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                            ret=send(new_sd,(void*)risposta,len,0);
                            if(ret<0){
                                perror("errore in fase di invio: \n"); 
                                close(new_sd);
                                exit(1);
                            } 
                            memset(risposta,0,sizeof(risposta));//pulisco  il buffer
                            //invio session_id
                            len=strlen(session_id)+1;//considero il caratter fine stringa
                            dim=htons(len);
                            ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                            ret=send(new_sd,(void*)session_id,len,0);
                            if(ret<0){
                                perror("errore in fase di invio: \n");
                                close(new_sd);
                                exit(1);
                            } 
                            //salvo username
                            strcpy(username_client,username);
                            break;
                        }                           
                    }
                   
                    fclose(fptr);
                    
                    if(!trovato){//utente non registrato
                       strcpy(risposta, "negativo");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                       tentativi++;
                       goto label3;
                    } 
                }
                
                
                if(!strcmp(op_code,"!invia_giocata")){
                    char sessionidricevuto[11];
                    char * puntatore_schedina;
                    char schedina [BUF_SIZE];
                    time_t rawtime;
                    struct tm* info;
                    FILE *fptr;
                    
                    printf("e' arrivata una schedina!\n");
                    //ricevo e controllo session id
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)sessionidricevuto,len,0);
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                        break;
                    } 
                    if(strcmp(session_id,sessionidricevuto)){//controllo validita session_id
                        printf("session_id non piu' valido\n");
                        strcpy(risposta, "session_id non piu' valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                        continue;
                    }
                    else{
                       strcpy(risposta, "session_id valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                    }
                    //ricevo schedina
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)schedina,len,0); 
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                        break;
                    } 
                    fptr=fopen(username_client,"a");
                    if (fptr  == NULL){
                         printf("Error! opening file\n");
                         exit(1);
                    }
                     time( &rawtime );
                     info = localtime( &rawtime );
                     fprintf(fptr,"giocata del:%s", asctime(info));
                     //ruote giocate
                     puntatore_schedina=strtok(schedina,"-");
                     fprintf(fptr,"ruote giocate:%s\n",puntatore_schedina+2);
                     //numeri giocati
                     puntatore_schedina=strtok(NULL,"-");
                     fprintf(fptr,"numeri giocati:%s\n",puntatore_schedina+2);
                     //importo puntate
                     puntatore_schedina=strtok(NULL,"-");
                     fprintf(fptr,"importo per tiologia di giocata.Rispettivamente estratto,ambo,terno,quaterna,cinquina:%s\n",puntatore_schedina+2);
                    
                    fclose(fptr);
                    //invio risposta
                    strcpy(risposta, "registrazione schedina andata a buon fine");
                    len=strlen(risposta)+1;
                    dim=htons(len);
                    ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    ret=send(new_sd,(void*)risposta,len,0);
                    memset(risposta,0,sizeof(risposta)); 
                    printf("schedina registrata per utente:%s\n",ip);
                }
                
                if(!strcmp(op_code,"!vedi_giocate")){
                    char sessionidricevuto[11];
                    int n,i;
                    float differenza;
                    char* pun;
                    char* eof;
                    uint16_t numero;
                    time_t t_giocata;
                    char stringa_t [BUF_SIZE];
                    char risposta [BUF_SIZE];
                    struct tm data_giocata;
                    FILE *fptr;
                    
                    printf("invio le sue giocate al client\n");
                    //ricevo e controllo session id
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)sessionidricevuto,len,0);
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                        break;
                    } 
                    if(strcmp(session_id,sessionidricevuto)){//controllo session_id
                        printf("session_id non piu' valido\n");
                        strcpy(risposta, "session_id non piu' valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                        continue;
                    }
                    else{
                       strcpy(risposta, "session_id valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                    }
                    //ricevo n
                    ret=recv(new_sd,(void*)&numero,sizeof(uint16_t),0);
                                   
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                       break;
                    } 
                    //converto n 
                    n=ntohs(numero);
                   // printf("%d\n",n);                                
                    fptr=fopen(username_client,"r");                
                    while((eof=fgets(stringa_t,BUF_SIZE,fptr))){
                        strptime(stringa_t+12,"%c", &data_giocata); // a +12 inizia la stringa della data
                        t_giocata=mktime(&data_giocata); 
                      //printf("%s\n",ctime(&t_giocata));
                        //printf("%s\n",ctime(ultima_estrazione)); 
                        sem_wait(mutex);
                        differenza=difftime(t_giocata,*ultima_estrazione);
                        sem_post(mutex);
                        //printf("%f\n",differenza);    
                            if(n==0){//invio le giocate relative a estrazioni gia' effettuate
                                if(differenza<0){//la giocata considerata è antecedente all'ultima estrazione
                                    i=0;
                                    while(i<3){
                                        fgets(stringa_t,BUF_SIZE,fptr);
                                        //printf("%s",stringa_t);
                                        if(i==0)
                                            pun=stringa_t+14;//mi posiziono a inizio elenco ruote
                                        if(i==1)
                                            pun=stringa_t+15; //mi posiziono a inizio elenco numeri giocati
                                        if(i==2)
                                            pun=stringa_t+strlen("importo per tipologia di giocata.Rispettivamente estratto,ambo,terno,quaterna,cinquina");//mi posiziono a inizio importi giocati
                                       
                                        len=strlen(pun);
                                        dim=htons(len);
                                        ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                                        ret=send(new_sd,(void*)pun,len,0);
                                        //printf("%s",pun);
                                        memset(stringa_t,0,sizeof(stringa_t));
                                        i++;
                                        //printf("%d\n",n);     
                                    }
                                }
                                else{
                                      //sono arrivato alle giocate attive
                                    strcpy(risposta,"fine giocata");
                                    len=strlen(stringa_t)+1;
                                    dim=htons(len);
                                    ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                                    ret=send(new_sd,(void*)risposta,len,0);
                                    memset(risposta,0,sizeof(risposta));
                                    break;
                                }
                            }
                            else{//cerco le giocate attive
                                if(differenza>0){//ho trovato una giocata attiva
                                    i=0;
                                    while(i<3){    
                                        fgets(stringa_t,BUF_SIZE,fptr);
                                       // printf("%s",stringa_t);
                                        if(i==0)
                                            pun=stringa_t+14;//mi posiziono a inizio elenco ruote
                                        if(i==1)
                                            pun=stringa_t+15; //mi posiziono a inizio elenco numeri giocati
                                        if(i==2)
                                            pun=stringa_t+strlen("importo per tiologia di giocata.Rispettivamente estratto,ambo,terno,quaterna,cinquina");//mi posiziono a inizio importi giocati
                                        len=strlen(stringa_t);
                                        dim=htons(len);
                                        //printf("%s",pun);
                                        ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                                        ret=send(new_sd,(void*)pun,len,0);
                                        memset(stringa_t,0,sizeof(stringa_t));
                                        i++;
                                    }
                                }
                                else{//passo alla prossima schedina da controllare
                                    int y;
                                    for(y=0;y<3;y++)//leggo tre righe per riposizionarmi all'inizio di una nuova giocata
                                        fgets(risposta,BUF_SIZE,fptr);
                                    strcpy(stringa_t,"");
                                }
                            }  
                    }
                   
                    if(!eof){//sono arrivato alla fine del file relativo alle schedine dell'utente
                        strcpy(risposta,"fine giocata");
                        len=strlen(risposta)+1;
                        dim=htons(len);
                        ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                        ret=send(new_sd,(void*)risposta,len,0);
                        memset(risposta,0,sizeof(risposta));
                    }
                   
                    fclose(fptr);
                }
                
                
                if(!strcmp(op_code,"!vedi_estrazione")){
                    char sessionidricevuto[11];
                    char* scan;
                    char  str[50];
                    FILE *fptr;
                    int n; //numero estrazioni richeste
                    char ruota[20];//nome ruota selezionata dal client
                    char num_ruota[10];//segue l'ordine delle ruote sulla schedina
                    
                    printf("invio le estrazioni al client\n");
                    //ricevo e controllo session id
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)sessionidricevuto,len,0);
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                        break;
                    } 
                    if(strcmp(session_id,sessionidricevuto)){//controllo validita' session_id
                       printf("session_id non piu' valido\n");
                       strcpy(risposta, "session_id non piu' valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                       continue;
                    }
                    else{
                       strcpy(risposta, "session_id valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                    }
                    //ricevo stringa
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)str,len,0);
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                        break;
                    } 
                    //recupero n(numero estrazioni)
                    scan=strtok(str," ");
                    n=atoi(scan);
                   // printf("%d\n",n);
                    //recupero ruota
                    scan=strtok(NULL," ");
                    if(!scan){//ruota non specificata
                        int x,y;
                        for(x=0;x<11; x++){//invio le estrazioni per ogni ruota
                            sprintf(num_ruota,"ruota%d",x);
                            fptr=fopen(num_ruota,"r");
                            int cicli=0;
                            sem_wait(sem);
                            if(n>*n_estrazioni)//controllo che il numero di estrazioni richieste sia minore del numero di estrazioni totale effettuate
                                n=*n_estrazioni;
                            else{
                                while(cicli<(*(n_estrazioni)-n)*2){//mi sposto sulle ultime n estrazioni
                                    fgets(risposta,BUF_SIZE,fptr);
                                    cicli++;
                                }
                            }
                             sem_post(sem);                           
                            for(y=0;y<n;y++){//invio le ultime n estrazioni
                                fseek(fptr,40,SEEK_CUR);//40 e' la dimensione della stringa della data    
                                memset(risposta,0,sizeof(risposta));
                                if(fgets(risposta,BUF_SIZE,fptr)==NULL)
                                    break;
                              //  printf("%s",risposta);
                                len=strlen(risposta)+1;
                                dim=htons(len);
                                ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                                ret=send(new_sd,(void*)risposta,len,0);  
                            }
                            fclose(fptr);
                        }
                        memset(risposta,0,sizeof(risposta));
                    }
                    else{//ruota specificata
                        int y;
                        strcpy(ruota,scan);
                        if(!strcmp(ruota,"bari"))
                             sprintf(num_ruota,"ruota%d",0);
                        if(!strcmp(ruota,"cagliari"))
                             sprintf(num_ruota,"ruota%d",1);
                        if(!strcmp(ruota,"firenze"))
                             sprintf(num_ruota,"ruota%d",2);
                        if(!strcmp(ruota,"genova"))
                             sprintf(num_ruota,"ruota%d",3);
                        if(!strcmp(ruota,"milano"))
                             sprintf(num_ruota,"ruota%d",4);
                        if(!strcmp(ruota,"napoli"))
                             sprintf(num_ruota,"ruota%d",5);
                        if(!strcmp(ruota,"palermo"))
                             sprintf(num_ruota,"ruota%d",6);
                        if(!strcmp(ruota,"roma"))
                             sprintf(num_ruota,"ruota%d",7);
                        if(!strcmp(ruota,"torino"))
                             sprintf(num_ruota,"ruota%d",8);
                        if(!strcmp(ruota,"venezia"))
                             sprintf(num_ruota,"ruota%d",9);
                        if(!strcmp(ruota,"nazionale"))
                             sprintf(num_ruota,"ruota%d",10);
                        
                        fptr=fopen(num_ruota,"r");                         
                        int cicli=0;
                            sem_wait(sem);
                            if(n>*n_estrazioni)
                                n=*n_estrazioni;
                            else{
                                while(cicli<(*(n_estrazioni)-n)*2){//come nel caso di sopra
                                    fgets(risposta,BUF_SIZE,fptr);
                                    cicli++;
                                }
                            }
                             sem_post(sem);                               
                        for(y=0;y<n;y++){
                            fseek(fptr,40,SEEK_CUR);  //mi sposto sui numeri  
                            memset(risposta,0,sizeof(risposta));
                            if(fgets(risposta,1000,fptr)==NULL)
                                break;
                           // printf("%s",risposta);
                            len=strlen(risposta)+1;
                            dim=htons(len);
                            ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                            ret=send(new_sd,(void*)risposta,len,0);  
                        }
                        fclose(fptr);
                        
                        
                    }
                }
                
                if(!strcmp(op_code,"!vedi_vincite")){
                    char sessionidricevuto[11];
                    char * puntatore_schedina;
                    char * pun;
                    char schedina [BUF_SIZE];//uso per leggere dati dalla schedina
                    char num_ruota[20];//stringa che contiene il nome del file che contiene l'estrazione per una ruota
                    float importi [5];//importi scommessi 
                    char estrazione_ruota [BUF_SIZE];//uso per leggere dati dalla ruota
                    int numeri_giocati[10];
                    int numeri_estratti[5];
                    char elenco_ruote[11][BUF_SIZE];//elenco ruote sui cui ho fatto una giocata
                    time_t t_giocata,t_prossima_estrazione;
                    struct tm time_giocata,time_prossima_estrazione;
                    int n_ruote_giocate,n_numeri_giocati;
                    char n_giocati [200];
                    char n_importi [200];
                    char e_ruote [BUF_SIZE];
                    float c_estratto=0.0;//consuntivo per tipologia di giocata 
                    float c_ambo=0.0;
                    float c_terna=0.0;
                    float c_quaterna=0.0;
                    float c_cinquina=0.0;
                    FILE * f_ruota,*f_schedina;
                    
                    printf("controllo vincite del client\n");
                    //ricevo e controllo session id
                    ret=recv(new_sd,(void*)&dim,sizeof(uint16_t),0);
                    len=ntohs(dim);
                    ret=recv(new_sd,(void*)sessionidricevuto,len,0);
                    if(ret<0){
                        perror("errore in fase di ricezione: \n");
                        break;
                    } 
                    
                    if(strcmp(session_id,sessionidricevuto)){
                        printf("session_id non piu' valido\n");
                        strcpy(risposta, "session_id non piu' valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                       continue;
                    }
                    else{
                        printf("session_id valido\n");
                        strcpy(risposta, "session_id valido");
                       len=strlen(risposta)+1;
                       dim=htons(len);
                       ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                       ret=send(new_sd,(void*)risposta,len,0);
                       memset(risposta,0,sizeof(risposta));
                       
                    }
                   //scorro tutte le schedine giocate
                    f_schedina=fopen(username_client,"r");
                    while(fgets(schedina,BUF_SIZE,f_schedina)){
                        float importo_vittoria [5];
                        bool VITTORIA=false;
                        int i;
                        strptime(schedina+12,"%c", &time_giocata); //data della schedina
                        t_giocata=mktime(&time_giocata); 
                        for(i=0;i<5;i++)
                            importo_vittoria[i]=0.0;
                        i=0;
                        while(i<3){//leggo i dati dalla schedina
                            //printf("while<3\n");
                            fgets(schedina,BUF_SIZE,f_schedina);
                            if(i==0){
                                int n=0;
                                puntatore_schedina=schedina+14;//mi posiziono a inizio elenco ruote
                                strcpy(e_ruote,puntatore_schedina);
                                pun=strtok(puntatore_schedina," ");

                                while(pun!=NULL && n<11){//copio elenco ruote
                                     strcpy(elenco_ruote[n],pun);
                                      pun=strtok(NULL," ");
                                     n++;
                                }
                                n_ruote_giocate=n;
                                
                                if(n_ruote_giocate<11)//elimino il carattere \n
                                    n_ruote_giocate--;
                                while(n<11){//inizializzo anche le locazioni vuote
                                    strcpy(elenco_ruote[n],"");
                                    n++;
                                }                               
                            }
                            if(i==1){
                                int h=0;
                                puntatore_schedina=schedina+15; //mi posiziono a inizio elenco numeri giocati
                                strcpy(n_giocati,puntatore_schedina);
                                pun=strtok(puntatore_schedina," ");
                                while(pun!=NULL&&h<10){
                                     numeri_giocati[h]=atoi(pun);
                                     pun=strtok(NULL," ");
                                     h++;
                                }
                                n_numeri_giocati=h;

                                if(n_numeri_giocati<10)//elimino il carattere \n
                                    n_numeri_giocati--;
                            }
                            if(i==2){//converto stringa di reali a array di float
                                int n=0;
                                puntatore_schedina=schedina+strlen("importo per tiologia di giocata.Rispettivamente estratto,ambo,terno,quaterna,cinquina:");
                                pun=strtok(puntatore_schedina," ");
                                while(pun!=NULL&&n<5){
                                     importi[n]=atof(pun);
                                     pun=strtok(NULL," ");
                                     n++;
                                }
                            } 
                            ++i;
                        }                                         
                        int num=0;
                       // printf("fine lettura schedina\n");
                        while(strlen(elenco_ruote[num])>1){
                           // printf("inizio scansione ruote\n");
                            if(!strcmp(elenco_ruote[num],"bari"))
                                 sprintf(num_ruota,"ruota%d",0);

                            if(!strcmp(elenco_ruote[num],"cagliari"))
                                 sprintf(num_ruota,"ruota%d",1);

                            if(!strcmp(elenco_ruote[num],"firenze"))
                                 sprintf(num_ruota,"ruota%d",2);

                            if(!strcmp(elenco_ruote[num],"genova"))
                                 sprintf(num_ruota,"ruota%d",3);

                            if(!strcmp(elenco_ruote[num],"milano"))
                                 sprintf(num_ruota,"ruota%d",4);

                            if(!strcmp(elenco_ruote[num],"napoli"))
                                 sprintf(num_ruota,"ruota%d",5);

                            if(!strcmp(elenco_ruote[num],"palermo"))
                                 sprintf(num_ruota,"ruota%d",6);

                            if(!strcmp(elenco_ruote[num],"roma"))
                                sprintf(num_ruota,"ruota%d",7);

                            if(!strcmp(elenco_ruote[num],"torino"))
                                 sprintf(num_ruota,"ruota%d",8);

                            if(!strcmp(elenco_ruote[num],"venezia"))
                                 sprintf(num_ruota,"ruota%d",9);

                            if(!strcmp(elenco_ruote[num],"nazionale"))
                                  sprintf(num_ruota,"ruota%d",10);

                            f_ruota=fopen(num_ruota,"r");

                        ruota:

                            memset(estrazione_ruota,0,sizeof(estrazione_ruota));
                            if(!fgets(estrazione_ruota,BUF_SIZE,f_ruota)){//sono arrivato alla fine delle estrazioni
                                fclose(f_ruota);
                                num++;
                                continue;
                            }
                            strptime(estrazione_ruota+15,"%c", &time_prossima_estrazione); //prendo time estrazione
                            t_prossima_estrazione=mktime(&time_prossima_estrazione);
                          /*
                            printf("%s",ctime(&t_prossima_estrazione));
                            printf("%f\n",difftime(t_prossima_estrazione,t_giocata));
                            printf("%f\n",difftime(t_giocata,t_ultima_giocata));*/
                            if(difftime(t_prossima_estrazione,t_giocata)>0){//controllo se siamo all'estrazione giusta
                                bool estratto=false;
                                bool ambo=false;
                                bool terna=false;
                                bool quaterna=false;
                                bool cinquina=false;
                                char leggi_numeri [20];
                                int h=0;
                               //ottengo numeri estratti
                                fgets(leggi_numeri,BUF_SIZE,f_ruota);
                              
                                pun=strtok(leggi_numeri," ");
                                while(pun!=NULL&&h<5){
                                     numeri_estratti[h]=atoi(pun);
                                     pun=strtok(NULL," ");
                                     h++;
                                }
                                //controllo vincite
                                int a,b;
                                for(a=0;a<n_numeri_giocati;a++){
                                    for(b=0;b<5;b++){
                                        if(numeri_giocati[a]==numeri_estratti[b]){
                                            if(estratto==false){
                                                estratto=true;
                                                continue;
                                            }
                                            if(ambo==false){
                                                ambo=true;
                                                continue;
                                            }
                                            if(terna==false){
                                                terna=true;
                                                continue;
                                            }
                                            if(quaterna==false){
                                                quaterna=true;
                                                continue;
                                            }
                                            if(cinquina==false){
                                                cinquina=true;
                                                continue;
                                            }
                                        }
                                    }
                                }
                                
                                if(estratto){
                                    importo_vittoria[0]+=importi[0]*(11.23/(combinazionisenzaripetizione(n_numeri_giocati,1)*n_ruote_giocate));
                                    c_estratto+=importi[0]*(11.23/(combinazionisenzaripetizione(n_numeri_giocati,1)*n_ruote_giocate));  
                                }
                                if(ambo){
                                    importo_vittoria[1]+=importi[1]*(250.0/(combinazionisenzaripetizione(n_numeri_giocati,2)*n_ruote_giocate));
                                    c_ambo+=importi[1]*(250.0/(combinazionisenzaripetizione(n_numeri_giocati,2)*n_ruote_giocate));
                                }
                                if(terna){
                                    importo_vittoria[2]+=importi[2]*(4500.0/(combinazionisenzaripetizione(n_numeri_giocati,3)*n_ruote_giocate));
                                    c_terna+=importi[2]*(4500.0/(combinazionisenzaripetizione(n_numeri_giocati,3)*n_ruote_giocate));
                                }
                                if(quaterna){
                                   importo_vittoria[3]+=importi[3]*(120000.0/(combinazionisenzaripetizione(n_numeri_giocati,4)*n_ruote_giocate));
                                    c_quaterna+=importi[3]*(120000.0/(combinazionisenzaripetizione(n_numeri_giocati,4)*n_ruote_giocate));
                                }
                                if(cinquina){
                                    importo_vittoria[4]+=importi[4]*(6000000.0/(combinazionisenzaripetizione(n_numeri_giocati,5)*n_ruote_giocate));
                                    c_cinquina+=importi[4]*(6000000.0/(combinazionisenzaripetizione(n_numeri_giocati,5)*n_ruote_giocate));
                                }
                               
                                VITTORIA=VITTORIA|(estratto&&importi[0]>0)|(ambo&&importi[1]>0)|(terna&&importi[2]>0)|(quaterna&&importi[3]>0)|(cinquina&&importi[4]>0);
                         
                            }
                            else{
                              //  printf("sono in else\n");
                                fgets(estrazione_ruota,BUF_SIZE,f_ruota);
                                goto ruota;
                            }
                            fclose(f_ruota); 
                            num++;
                        }
                        if(VITTORIA){
                            char informazioni [BUF_SIZE];
                            int ciclo=0;
                            printf("il client ha vinto\n");
                            //invio schedina che contiene una o piu' vincite
                            while(ciclo<4){
                                if(ciclo==0)
                                    strcpy(informazioni,asctime(&time_prossima_estrazione));                                    
                                if(ciclo==1)
                                    strcpy(informazioni,e_ruote);
                                if(ciclo==2)
                                    strcpy(informazioni,n_giocati);
                                if(ciclo==3){
                                    int index=0;
                                    int indice=0;
                                    char importo[20];
                                    while(indice<5){  
                                        snprintf(importo,50,"%f  ",importo_vittoria[indice]);
                                        strcpy(n_importi+index,importo);
                                        index+=strlen(importo);
                                        indice++;
                                    }
                                   // printf("%s",n_importi);
                                    strcpy(informazioni,n_importi);
                            }
                             len=strlen(informazioni)+1;
                            dim=htons(len);
                            ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                            ret=send(new_sd,(void*)informazioni,len,0);
                            if(ret<0){
                                perror("errore in fase di invio:");
                                close(new_sd);
                                exit(1);
                            }
                            ciclo++;
                            }
                        }
                   }
                    fclose(f_schedina);
                    int r=0;
                    while(r<6){//invio totale vincite per tipologia
                        memset(risposta,0,sizeof(risposta));
                        if(r==0)
                            strcpy(risposta,"fine lettura schedine");
                        if(r==1)
                            gcvt(c_estratto,8,risposta);
                        if(r==2)
                            gcvt(c_ambo,8,risposta);
                        if(r==3)
                            gcvt(c_terna,8,risposta);
                        if(r==4)
                            gcvt(c_quaterna,8,risposta);
                        if(r==5)
                            gcvt(c_cinquina,8,risposta);

                        len=strlen(risposta)+1;
                        dim=htons(len);
                        ret=send(new_sd,(void*)&dim,sizeof(uint16_t),0);
                        ret=send(new_sd,(void*)risposta,len,0);
                        if(ret<0){
                            printf("Impossibile inviare op_code\n");
                            close(new_sd);
                            exit(1);
                         }
                        r++;
                    }
                }
                
              
                
            }
            close(new_sd);
            exit(1);
        }
        else close(new_sd);
    
    }
    
    return (EXIT_SUCCESS);
}

