#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define MAX_SIZE 1024
#define CMD_SIZE 100
 
int main(int argc, char** argv) {
    
    if(argc!=3){
        printf("Paramentri per la configurazione del client non validi \n");
        return(0);
    }
    
    
    int srv_port=atoi(argv[2]);
    char srv_ip[sizeof(argv[1])];
    strcpy(srv_ip,argv[1]);
    
    char session_id[11];
    int ret, sd, len;
    uint16_t lmsg;
    struct sockaddr_in srv_addr;
    char cmd[CMD_SIZE];//stringa dei comandi da inviare
    char buf_s[MAX_SIZE];
    char delim[] = "\n, ";
    /* Creazione socket */
    sd = socket(AF_INET, SOCK_STREAM, 0);
    /* Creazione indirizzo del server */
    memset(&srv_addr, 0, sizeof(srv_addr)); // Pulizia 
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(srv_port);
    inet_pton(AF_INET, srv_ip, &srv_addr.sin_addr);
    
    printf("**********************GIOCO DEL LOTTO*********************** \n");
    printf("Sono disponibili i seguenti comandi: \n");
    printf("\n");
    printf("1) !help <comando> --> mostra i dettagli di un comando\n");
    printf("2) !signup <username> <password> --> crea un nuovo utente\n");
    printf("3) !login <username> <password> --> autentica un utente\n");
    printf("4) !invia_giocata <g> --> invia una giocata g al server\n");
    printf("5) !vedi_giocate <tipo> --> visualizza le giocate precedenti\n "
            "dove tipo={0,1} e permette di visualizzare le giocate passate '0' \n"
            " oppure le giocate attive'1' (ancora non estratte) \n");
    printf("6) !vedi_estrazione <n> <ruota> --> mostra i numeri delle ultime \n"
            " n estrazioni sulla ruota specificata\n");
    printf("7) !vedi_vincite --> mostra tutte le vincite del client \n");
    printf("8) !esci --> termina il client \n");
    
    memset(session_id,1,sizeof(session_id));
    ret = connect(sd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    
    if(ret < 0){
        perror("Errore in fase di connessione: \n");
        exit(1);
    }
    while(1){
        printf("\n> ");
        memset(cmd,0,sizeof(cmd));
        fgets(cmd,CMD_SIZE,stdin); //leggo tutto il buffer, spazi inclusi
        if(strlen(cmd)==1)
            continue;
        char *scan = strtok(cmd, delim); //spezzo l'array
        char *ptr=scan;
       
               
        if(!strcmp(ptr,"!help")){//comando help
   
            scan=strtok(NULL,delim);
            if(!scan) {
                printf("Sono disponibili i seguenti comandi: \n");
                printf("\n");
                printf("1) !help <comando> --> mostra i dettagli di un comando\n");
                printf("2) !signup <username> <password> --> crea un nuovo utente\n");
                printf("3) !login <username> <password> --> autentica un utente\n");
                printf("4) !invia_giocata <g> --> invia una giocata g al server\n");
                printf("5) !vedi_giocate <tipo> --> visualizza le giocate precedenti\n "
                            "dove tipo={0,1} e permette di visualizzare le giocate passate '0' \n"
                            " oppure le giocate attive'1' (ancora non estratte) \n");
                printf("6) !vedi_estrazione <n> <ruota> --> mostra i numeri delle ultime \n"
                            " n estrazioni sulla ruota specificata\n");
                printf("7) !vedi_vincite --> mostra tutte le vincite del client \n");
                printf("8) !esci --> termina il client \n");     
                      
            }
            else{  
                if(!strcmp(scan,"!help")){
                    printf("Ti sto dando una mano!\n");
            }
            else if(!strcmp(scan,"!signup")) {      
                    printf("crea un nuovo utente\n");  
                }
            else if(!strcmp(scan,"!login")){
                    printf("autentica un utente\n");          
                }
            else if(!strcmp(scan,"!invia_giocata")){
                    printf("invia una giocata g al server\n"); 
                }
            else if(!strcmp(scan,"!vedi_giocate")){
                    printf("visualizza le giocate precedenti\n "
                           "dove tipo={0,1} e permette di visualizzare le giocate passate '0' \n"
                           " oppure le giocate attive'1' (ancora non estratte) \n");
                }
            else if(!strcmp(scan,"!vedi_estrazione")){
                    printf(" mostra i numeri delle ultime \n"
                           " n estrazioni sulla ruota specificata");
                }
            else if(!strcmp(scan,"!vedi_vincite")){
                    printf(" mostra tutte le vincite del client \n");
                }
            else if(!strcmp(scan,"!esci")){
                    printf("termina il client \n");
                }
            }
        }
           
        if(!strcmp(ptr,"!signup")){//comando signup
                char esito [BUFFER_SIZE];
                int len;
                int tentativo=0;
                //invio op_code
                len=strlen(ptr)+1;
                lmsg=htons(len);
                ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                ret=send(sd,(void*)ptr,len,0);
                if(ret<0){
                        printf("Impossibile inviare op_code\n");
                        close(sd);
                        exit(1);
                }
        label1: 
                if(tentativo==0){ //primo tentativo
                    //ottengo la stringa con usr, pwd
                    scan=strtok(NULL,"/");
                    len=strlen(scan)+1;
                    lmsg=htons(len);
                    ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                    ret=send(sd,(void*)scan,len,0);
                    if(ret<0){
                        printf("Impossibile inviare credenziali\n");
                        close(sd);
                        exit(1);
                    }
                   // printf("invio usr pwd\n");
                    tentativo++;
                }
                else{
                    printf("inserire username e password: \n");
                    fgets(buf_s,BUFFER_SIZE,stdin);
                    //invio credenziali
                    len=strlen(buf_s)-1;//non voglio inviare il carattere \n
                    printf("%d\n",len);
                    lmsg=htons(len);
                    ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                    ret=send(sd,(void*)buf_s,len,0);
                    if(ret<0){
                        printf("Impossibile inviare credenziali\n");
                        close(sd);
                        exit(1);
                    }
                    printf("invio usr pwd\n");
                }
                //aspetto risposta dal server
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)esito,len,0);
                if(!strcmp(esito,"negativo")){
                    printf("username non disponibile! \n");
                    goto label1;
                }
                else{
                    printf("%s\n",esito);
                }       
        }
        
        if(!strcmp(ptr,"!login")){
                char esito[50];
                int tentativi=0;
                //invio op_code
                len=strlen(ptr)+1;
                lmsg=htons(len);
                ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                ret=send(sd,(void*)ptr,len,0);
                if(ret<0){
                        printf("Impossibile inviare op_code\n");
                        close(sd);
                        exit(1);
                }
                
                
          label2:if(tentativi==3){
                 printf("tentaivi esauriti, riprova tra 30 minuti\n");
                 close(sd);
                 exit(1);    
                }
                if(tentativi==0){
                    //invio credenziali
                    scan=strtok(NULL,"\n");
                    //printf("%s\n",scan);
                    len=strlen(scan)+1;
                    lmsg=htons(len);
                    ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                    ret=send(sd,(void*)scan,len,0);
                    if(ret<0){
                              printf("Impossibile inviare credenziali\n");
                              close(sd);
                              exit(1);
                    }

                      tentativi++; 
                }
                else{
                   
                    printf("inserire username e password: \n");
                    fgets(buf_s,BUFFER_SIZE,stdin);
                    len=strlen(buf_s)-1;//non voglio inviare il carattere \n
                   // printf("%d\n",len);
                    lmsg=htons(len);
                    ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                    ret=send(sd,(void*)buf_s,len,0);
                    if(ret<0){
                        printf("Impossibile inviare credenziali\n");
                        close(sd);
                        exit(1);
                    }
                    printf("invio credenziali\n");
                    memset(buf_s,0,sizeof(buf_s));
                    tentativi++;
                }
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)esito,len,0);
                if(ret<0){
                     printf("errore in fase di ricezione \n");
                     close(sd);
                     exit(1);
                }
                
                if(!strcmp(esito,"ip bloccato")){
                    printf("ti sei collegato troppo presto, riprova tra 30 minuti\n");
                    close(sd);
                    exit(1);
                    
                }
                if(!strcmp(esito,"negativo") ){ //posso tentare al piu' 3 volte
                      printf("credenziali non valide,inserisci di nuovo\n");
                      //pulisco
                      memset(esito,0,sizeof(esito));
                      goto label2;
                }
                        
                //ricevo session_id
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)session_id,len,0);
                
               // printf("%s\n",session_id);
                printf("autentificazione avvenuta con successo\n");
                
        }
        
        if(!strcmp(ptr,"!invia_giocata")){
            char esito[50];
            //invio op_code
            len=strlen(ptr)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)ptr,len,0);
            if(ret<0){
                    printf("Impossibile inviare op_code\n");
                    close(sd);
                    exit(1);
            }
            //invio session_id
            len=strlen(session_id)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)session_id,len,0);
            if(ret<0){
                printf("Impossibile inviare session_id\n");
                close(sd);
                exit(1);
            }
            //controllo se session_id è stato riconosciuto
            memset(esito,0,sizeof(esito));
            ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
            len=ntohs(lmsg);
            ret=recv(sd,(void*)esito,len,0);
            if(ret<0){
                printf("Impossibile ottenere risposta dal server\n");
                close(sd);
                exit(1);
            }
           // printf("%s\n",esito);
            if(!strcmp(esito,"session_id non piu' valido"))
                continue;
            else{
                //invio schedina. Nella schedina elenco tutte le ruote giocate e specifico tutti gli importi giocati e non giocati(metto ugualmente 0 per le giocate che non faccio)
                scan=strtok(NULL,"\n");
               // printf("%s",scan);
                len=strlen(scan)+1;
                lmsg=htons(len);
                ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                ret=send(sd,(void*)scan,len,0);
                if(ret<0){
                    printf("Impossibile inviare schedina\n");
                    close(sd);
                    exit(1);
                }
                //aspetto notifica dal server
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)esito,len,0);
                printf("%s\n",esito);
            }
        }
        
        if(!strcmp(ptr,"!vedi_giocate")){
            int n,n_giocata=0;
            uint16_t numero;
            char esito[50];
            char stringa_giocata[100];
            char buf[BUFFER_SIZE];
            //controllo parametro n
            scan=strtok(NULL,"/");
            if(scan==NULL){
                printf("parametro non specificato:{0,1}\n");
                continue;
            }
            
            //invio op_code
            len=strlen(ptr)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)ptr,len,0);
            if(ret<0){
                    printf("Impossibile inviare op_code\n");
                    close(sd);
                    exit(1);
            }
            //invio session_id
            len=strlen(session_id)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)session_id,len,0);
            if(ret<0){
                printf("Impossibile inviare session_id\n");
                close(sd);
                exit(1);
            }
            //controllo validita' session_id
            memset(esito,0,sizeof(esito));
            ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
            len=ntohs(lmsg);
            ret=recv(sd,(void*)esito,len,0);
             if(ret<0){
                printf("Impossibile ottenere risposta dal server\n");
                close(sd);
                exit(1);
            }
           // printf("%s\n",esito);
            if(!strcmp(esito,"session_id non piu' valido"))
                continue;
            else{
                //invio numero {0,1}
                n=atoi(scan);
                numero=htons(n);
                ret=send(sd,(void*)&numero,sizeof(uint16_t),0);
                if(ret<0){
                    printf("Impossibile inviare n\n");
                    close(sd);
                    exit(1);
                }    
            }
            while(1){
                int i;
                int index;
                //ricevo giocata non presente o elenco ruote della giocata
                memset(stringa_giocata,0,sizeof(stringa_giocata));
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)stringa_giocata,len,0);
               // printf("%s",stringa_giocata);
                if(ret<0){
                    printf("Impossibile ottenere conferma di giocata dal server\n");
                    close(sd);
                    exit(1);
                }
               
                if(!strcmp(stringa_giocata,"fine giocata")){//non ho piu' giocate da ricevere
                    printf("fine schedine giocate\n");
                    break;
                }
                //copio ruote giocate
                index=0;
                strcpy(buf+index,stringa_giocata); 
                index=index+strlen(stringa_giocata)-1;//levo \n dalla stringa ricevuta
               // printf("%s",buf);
                i=0;
                while(i<2){//ricevo numeri giocati e importi giocati
                    memset(stringa_giocata,0,sizeof(stringa_giocata));
                    ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                    len=ntohs(lmsg);
                    ret=recv(sd,(void*)stringa_giocata,len,0);
                     if(ret<0){
                        printf("Impossibile ottenere parte di stringa di una giocata\n");
                        close(sd);
                        exit(1);
                    }
                    //copio numeri giocati
                    if(i==0){
                      strcpy(buf+index,stringa_giocata); 
                      index=index+strlen(stringa_giocata)-1;//levo \n dalla stringa ricevuta
                      //printf("%s",buf);
                    }
                    else{
                        char* token;
                        int tipo_giocata=0;
                        token=strtok(stringa_giocata," ");//spezzo la stringa degli importi giocati
                        while(token!=NULL){
                           if(tipo_giocata==0){
                               strcpy(buf+index," * estratto ");
                               index=index+strlen(" * estratto ");
                           } 
                           if(tipo_giocata==1){
                               strcpy(buf+index," * ambo :");
                               index=index+strlen(" * ambo :");
                           }
                           if(tipo_giocata==2){
                               strcpy(buf+index," * terno :");
                               index=index+strlen(" * terno :");
                           }
                           if(tipo_giocata==3){
                               strcpy(buf+index," * quaterna :");
                               index=index+strlen(" * quaterna :");
                           }
                           if(tipo_giocata==4){
                               strcpy(buf+index," * cinquina :");
                               index=index+strlen(" * cinquina :");
                           }
                           strcpy(buf+index,token); 
                           index=index+strlen(token);
                           token=strtok(NULL," ");
                           tipo_giocata++;
                        }
                    }  
                    i++;
                }
                n_giocata++;
                printf("%d)%s",n_giocata,buf);
            }  
        }
        
        if(!strcmp(ptr,"!vedi_estrazione")){
            char esito[100];
            char estrazione[100];
            char* pun;
            int n,indice,rimanenti;
            char stringainvio[20];
            //invio op_code
            len=strlen(ptr)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)ptr,len,0);
            if(ret<0){
                    printf("Impossibile inviare op_code\n");
                    close(sd);
                    exit(1);
            }
            //invio session_id
            len=strlen(session_id)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)session_id,len,0);
            if(ret<0){
                printf("Impossibile inviare session_id\n");
                close(sd);
                exit(1);
            }
            //controllo validita' session_id
            memset(esito,0,sizeof(esito));
            ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
            len=ntohs(lmsg);
            ret=recv(sd,(void*)esito,len,0);
             if(ret<0){
                printf("Impossibile ottenere risposta dal server\n");
                close(sd);
                exit(1);
            }
            //printf("%s\n",esito);
            if(!strcmp(esito,"session_id non piu' valido")){
                printf("session_id non più valido\n");
                continue;
            }    
            else{
                
                //invio n e ruota se presente
                scan=strtok(NULL,"\n");
                strcpy(stringainvio,scan);
                len=strlen(scan)+1;
                lmsg=htons(len);
                
                //printf("%s\n",scan);
                ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
                ret=send(sd,(void*)stringainvio,len,0);
                if(ret<0){
                    printf("Impossibile inviare stringa\n");
                    close(sd);
                    exit(1);
                }
                rimanenti=0;
                //ottengo n e ruota
                pun=strtok(stringainvio," ");
                n=atoi(pun);
               // printf("%d\n",n);
                //recupero ruota
                pun=strtok(NULL," ");
               // printf("%s\n",pun);
                if(!pun){//non ho specificato una ruota
                    for(indice=0;indice<11;indice++){
                        if(indice==0)
                            printf("Bari      ");
                        if(indice==1)
                            printf("Cagliari  ");
                        if(indice==2)
                            printf("Firenze   ");
                        if(indice==3)
                            printf("Genova    ");
                        if(indice==4)
                            printf("Milano    ");
                        if(indice==5)
                            printf("Napoli    ");
                        if(indice==6)
                            printf("Palermo   ");
                        if(indice==7)
                            printf("Roma      ");
                        if(indice==8)
                            printf("Torino    ");
                        if(indice==9)
                            printf("Venezia   ");
                        if(indice==10)
                            printf("Nazionale ");
                        rimanenti=0;
                        while(rimanenti<n){//ricevo e stampo estrazioni
                            memset(estrazione,0,sizeof(estrazione));
                            ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                            len=ntohs(lmsg);
                            ret=recv(sd,(void*)estrazione,len,0);
                            printf("%s",estrazione);
                            printf("          ");
                            rimanenti++;
                        }
                        printf("\n");
                    }
                }
                else{
                    printf("%s\n",pun);
                    while(rimanenti<n){//ricevo e stampo estrazioni
                         memset(estrazione,0,sizeof(estrazione));
                         ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                         len=ntohs(lmsg);
                         ret=recv(sd,(void*)estrazione,len,0);
                         printf("%s",estrazione);
                         rimanenti++;
                        }
                    
                }  
            }
        }
        
        if(!strcmp(ptr,"!vedi_vincite")){
            char esito[BUFFER_SIZE];
            char buf[BUFFER_SIZE];
            int index=0;
            int n=0;
            //invio op_code
            len=strlen(ptr)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)ptr,len,0);
            if(ret<0){
                    printf("Impossibile inviare op_code\n");
                    close(sd);
                    exit(1);
            }
            //invio session_id
            len=strlen(session_id)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)session_id,len,0);
            if(ret<0){
                printf("Impossibile inviare session_id\n");
                close(sd);
                exit(1);
            }
            //ricevo esito controllo
            memset(esito,0,sizeof(esito));
            ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
            len=ntohs(lmsg);
            ret=recv(sd,(void*)esito,len,0);
            if(ret<0){
                printf("Impossibile ottenere risposta dal server\n");
                close(sd);
                exit(1);
            }
           // printf("%s\n",esito);
            if(!strcmp(esito,"session_id non piu' valido")){
                printf("session_id non piu' valido\n");
                continue;
            }    
            //ricevo dati sulle vincite
            while(1){
              //  printf("inizio\n");
                memset(esito,0,sizeof(esito));
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)esito,len,0);
                 if(ret<0){
                    printf("Impossibile ottenere risposta dal server\n");
                    close(sd);
                    exit(1);
                }
                if(!strcmp(esito,"fine lettura schedine"))
                    break;
                if(n==0){//copio stringa data estrazione
                    printf("Estrazione del:%s",esito);
                    memset(buf,0,sizeof(buf));
                    index=0;
                }
                if(n==1||n==2){//concateno ruote giocate,numeri giocati 
                    strcpy(buf+index,esito);
                    index+=strlen(esito)-1;
                    if(n==2){
                        strcpy(buf+index,"  >>  ");
                        index+=strlen("  >>  ");
                    }    
                }
                if(n==3){//concateno valore vincita per tipologia di giocata, nessuna esclusa
                    char* token;
                    int tipo_giocata=0;
                    token=strtok(esito," ");
                    while(token!=NULL){
                           if(tipo_giocata==0){
                               strcpy(buf+index,"  estratto ");
                               index=index+strlen("  estratto ");
                           } 
                           if(tipo_giocata==1){
                               strcpy(buf+index,"  ambo ");
                               index=index+strlen("  ambo ");
                           }
                           if(tipo_giocata==2){
                               strcpy(buf+index,"  terno ");
                               index=index+strlen("  terno ");
                           }
                           if(tipo_giocata==3){
                               strcpy(buf+index,"  quaterna ");
                               index=index+strlen("  quaterna ");
                           }
                           if(tipo_giocata==4){
                               strcpy(buf+index,"  cinquina ");
                               index=index+strlen("  cinquina ");
                           }
                           strcpy(buf+index,token); 
                           index=index+strlen(token);
                           token=strtok(NULL," ");
                           tipo_giocata++;
                        }
                    printf("%s",buf);
                    printf("\n****************************************************\n");
                }
                n=(n+1)%4;
               // printf("%d",n);
            }
            n=0;
          //  printf("fine\n");
            while(n<5){
                //ricevo e stampo i resoconti delle vincite per tipologia di giocata 
                memset(esito,0,sizeof(esito));
                ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
                len=ntohs(lmsg);
                ret=recv(sd,(void*)esito,len,0);
                 if(ret<0){
                    printf("Impossibile ottenere risposta dal server\n");
                    close(sd);
                    exit(1);
                }
            
                if(n==0)
                    printf("Vincite su ESTRATTO:%s\n",esito);
                if(n==1)
                    printf("Vincite su AMBO:%s\n",esito);
                if(n==2)
                    printf("Vincite su TERNO:%s\n",esito);
                if(n==3)
                    printf("Vincite su QUATERNA:%s\n",esito);
                if(n==4)
                    printf("Vincite su CINQUINA:%s\n",esito);
                n++;
            }
            
            
            
        }
        
        if(!strcmp(ptr,"!esci")){
            char esito[50];
            len=strlen(ptr)+1;
            lmsg=htons(len);
            ret=send(sd,(void*)&lmsg,sizeof(uint16_t),0);
            ret=send(sd,(void*)ptr,len,0);
            if(ret<0){
                printf("Impossibile inviare op_code\n");
                close(sd);
                exit(1);
                }
            ret=recv(sd,(void*)&lmsg,sizeof(uint16_t),0);
            len=ntohs(lmsg);
            ret=recv(sd,(void*)esito,len,0); 
            printf("%s\n",esito);
            close(sd);
            exit(1);
        }
          
        
       
    }
    close(sd);

    
    return (EXIT_SUCCESS);

    }
