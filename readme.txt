Tica Ion Valentin - 322CB

Tema 2 PCom -- Aplicatie client-server TCP si UDP pentru
gestionarea mesajelor


  Inceputul acestei teme a fost destul de greu, spre deosebire de asteptarile
pe care le aveam, deoarece ni se spusese ca e mai usoara :)) dar pentru mine 
a fost nevoie de mai multa documentatie inainte. Atat din laboratoare, tutorialele
oferite despre udp, tcp si alte surse online intelegand apoi mai bine.

  Principiul temei este descris conform acelui grafic din cerinta in care serverul este
elementul de mijloc, deschizand 2 socketi pentru clientul tcp si pt clientul udp folosind
comenzile pentru a seta optiunile. Comenzi invatate la laborator si in practica.
  Adaugam socketii in file descriptor si tastatura de la care putem primi comenzi de un/subscribe si exit. In ciclicitate verificam daca primim mai intai o conexiune noua de la clientul tcp. Apoi incepem verificari daca exista sau nu si sa facem operatiile aferente. 
Daca ne vine un client tcp, verificam daca exista in vectorul de clienti tcp, daca nu il adaugam, daca exista vedem daca e online si in caz contrar il facem si actualizam socketul
prin care a primit si afiseaza.
  Atunci cand primim un client udp trimite un mesaj ce contine topicul si informatiile sale,
impreuna cu parametrul cu store and forward.

  In subscriber avem 2 lucruri de efectuat, de primit mesajele udp trimise de catre server mai
departe catre clientii tcp si sa afisam la stdout informatiile conform cerintei sau sa primim acele comenzi de la stdin. Cream file descriptor-ul pe care adaugam socket-ul si stdin-ul si trimitem catre server id-ul clientului care se va conecta.
