// CodeGeckoV5.ino
// V4->V5 course_min a été mis à 296 au lieu de 380 permet peut-être d'avoir un meilleure vibro
// V3 -> V4 Elimine le full stroke quand on débranche la Télécommande en pleine action. 
// Comme le fait de lire la vitesse du potentiomètre prend relativement beaucoup de temps, 
// cela réduit la vitesse de pulsation de la pin 9. Pour parer à ceci le programme a été adapté
// pour lire la vitesse tous les 300 itérations à basse vitesse.
// Lorsque la vitesse du potentiomètre dépasse v_max_inv qui est la vitesse maximale à laquelle
// le moteur ne décroche pas à l'inversion du sens de rotation, des boucles d'accélérations et de
// décélération sont requises pour ne pas faire décrocher le moteur lors des inversions de sens de
// rotations. (avec ceci, il est possible de diminuer le délais entre pulse à haute vitesse ce
// qui augmente la vitesse) Comme la vitesse est rapide la vitesse est alors lue après un cycle
// complet de va-et-vient. Pour simplifier, la course n'est lue qu'après un cycle complet de va-et-vient.
// Un autre point important est de lire la vitesse souvent à basse vitesse : 
// par exemple si la vitesse est au mini et la course est grande, alors cela prendra une éternité
// pour que le va-et-vient se termine jusqu'à la prochaine lecture de vitesse.
// L'accélération et la décélération est linéaire (après un cycle de pulse, le délais est soustrait pour
// l'accélération ou additioné de 1 pour la décélération).

#define step_pin 9          // Pin 9 connected to Steps pin on ST-M5045 ou ... c'est la pin du pulse 
                            // (un pulse c'est un pas en mode full 
                            // ou en mode 1600pas/rev : 8 pulses pour un pas avec un motor de 1.8deg 
                            // ou 200pas/rev)
#define dir_pin 8           // Pin 8 connected to Direction pin
#define home_switch 12      // Pin 12 connected to Home Switch (MicroSwitch)
#define ref 0               // c'est le 0 ici l'inversion est faite en mode basse vitesse 
                            // (sans accélération ni décélération)

//****************************************************************************************************
//**********************PARAMETRES MODIFIABLES********************************************************
#define v_max 40            // C'est la vitesse maximale lue par le potentiomètre de vitesse (délai en
                            // microseconde entre deux pulses) 30 50 45 55 65 70 65 Valeur original 50.
#define v_min 1200          // C'est la vitesse minimum lue par le potentiomètre de vitesse (délai en 
                            // microseconde entre deux pulses)
#define v_max_inv 200       // C'est la vitesse maximale à laquelle le moteur ne décroche pas 200 280
                            // Valeur précédente=200 à l'inversion de sens de rotation (200 pour Gecko
                            // et 200 pour 1600pas/rev?). En dessous de cette vitesse, l'accélération 
                            // et la décélération ne sont pas requise. C'est pourquoi la course peut être 
                            // plus petite car il n'y pas besoin de distance de freinage. A cette petite
                            // course le va-et-vient est tellement rapide que ça vibre. On retrouve ici
                            // le mode vibro.
#define course_max 2968     // C'est la course maximale lue par le potentiomètre de course 2150 2125 
                            // 2100 2080 origine 2320 2500 Cette valeur dépende de la longueur de la 
                            // machine.
                            
#define marge 30            // C'est la marge ou décalage de la référence par rapport à la position du 
                            // bras à la mise sous tension de la machine en principe en butée du coté 
                            // vert ou coté rouge. Ceci assure que la partie mobile ne viennent cogné 
                            // sur l'un des cotés. Valeur originale = 50.
//*****************************************************************************************************
int course_min = 296;       // C'est la course minimum lue par le potentiomètre de course. Qui corresond
                            // à la course minimum pour l'accélération et la décélération à haute vitesse. 
                            // (vitesse à laquelle la distance de freinage/accélération est requise). 
                            // Soit course_min > 2*(v_max_inv - v_max) = distance max de freinage ou 
                            // d'accélération.

                           // Si on est dans le mode basse vitesse alors on peut réduire la course pour 
                           // le mode vibro.
int diff_course = 1200;    // C'est la tolérance d'une différence de course. Cette valeur est utilisée pour 
                           // comparer la valeur précédente de la course (customCourseMapped_pre) et la 
                           // valeur courante. S'il y avait un problème de mesure du poto cela évite que 
                           // la course se mette au max, ce qui pourrait être dangereux.                   
int x=v_max_inv;           // Variable de délai entre impulsion qui varie selon accéleration et décélération
int steps=0;               // Variable compteur de pas. Sert à savoir la position du bras (position courante)
int steps_init=0;          // Variable compteur de pas. Pour la prise de référence à l'initialisation
                
int analog_read_counter = 300; // Permet de pas lire trop souvent la vitesse du potentiometre. Ce qui permet
                               // une vitesse plus élevée du va-et-vient.

int customCourseMapped;        // Pour stocker la consigne de la course courante
int customCourseMapped_pre;    // Pour stocker la consigne de la course précédente (pour controler que la diff
                               // n'est pas trop grande (sécurité)
int customSpeedMapped;         // Pour stocker la consigne de la vitesse
int dist_frein = v_max_inv-v_max; // C'est la vitesse de freinage. On initialise à la valeur max (quand la vitesse est la plus grande)

bool CoteVert = false;   // Pour définir le côté vert avec les potos
bool CoteRouge = false;  // Pour définir le côté rouge avec les potos
bool CoteMilieu = false; // Pour définir l'utilisation à deux de la machine (les bras se mettent au milieux. 

void setup() {                  
  pinMode(dir_pin, OUTPUT);      
  pinMode(step_pin, OUTPUT); 
  digitalWrite(dir_pin, LOW);
  digitalWrite(step_pin, LOW);

  //Serial.begin(9600);

  pinMode(home_switch, INPUT_PULLUP);
  
  delay(1000);  // on attend un peu
   
  steps=-marge;  // on rajoute un poil de cul plié en quatre pour que le 0 soit un peu plus loin
                 // on aura, avant la mise sous tension de la machine, poussé le bras en butée à
                 // l'intérieur de la machine du côté vert ou rouge (côté duquel on veut se mettre)

  //*******************************************************************************************
  // ici on vérifie la position des potos afin de savoir de quel côté la machine est utilisée.
  //*******************************************************************************************
  
  //ici la machine est utilisée du côté vert
  
  if(speedUp() > 1100 && course() < 400){
  CoteVert = true; 
  CoteRouge = false;}
  
  //ici la machine est utilisée du côté rouge
  
  if(speedUp() < 100 && course() > 2600){
    CoteRouge = true;
    CoteVert = false;
  }
  
  //ici la machine est utilisée sur les deux côtés à la fois
  
  if((speedUp() > 400 && speedUp() < 800) && (course() > 1290 && course() < 2090)){
    CoteMilieu = true;
    CoteVert = false;
    CoteRouge = false;
  }

  if(CoteMilieu){
    while(steps<(course_max/2)){  // va jusqu'au milieu
      digitalWrite(step_pin, HIGH);
      delay(1);          
      digitalWrite(step_pin, LOW); 
      delay(1);
      steps++;
    }
  }

  // tant que le potentiomètre de la vitesse et de la course ne sont pas au minimum, on ne démarre pas (pour la sécurité)
  
  while (!(speedUp() > (v_min - 30) && course() < 400)){ 
  }
}

void loop() {
 
  customSpeedMapped =speedUp(); // lecture de la vitesse
  customCourseMapped = course(); // lecture de la course
  if (((customCourseMapped-customCourseMapped_pre) > diff_course) && (customSpeedMapped < 200)) {
    while(1){} // On rentre en boucle infinie (pour la sécurité évite de se faire percer trop violemment)
  }

  
  //*********************************************Ne sait pas trop ce que ça fait?**************************
  while ((customSpeedMapped) > (v_min - 17)){ // si le potentiomètre de la vitesse est au minimum, 
                                              //on met LM42P à l'arrêt
    if (analog_read_counter > 0) { //pour ne pas lire trop souvent le pot de la vitesse
      analog_read_counter--;
    }
    else {
      analog_read_counter = 400;
      customSpeedMapped =speedUp(); // permet de modifier la vitesse sans devoir attendre un va complet
    }
  } 
 //*********************************************************************************************************


 if(CoteVert){
  if(customSpeedMapped<v_max_inv){
    dist_frein = v_max_inv-customSpeedMapped;                               // On recalcule la distance de freinage pour la vitesse de consigne actuelle.
    course_min = 380;      //course min plus grande                         // Plus la vitesse est élévée et plus la distance de freinage est longue.
  }                                                                         // Rappel : une valeur petite pour la vitesse signifie une grande vitesse
                                                                            // puisque la valeur est le délai en microsecondes entre-pulse                                                                                           
  else{course_min = 30;} // fonction vibro à basse vitesse
  
 // customCourseMapped = course(); // on lit la course en fonction vibro ou pas***********************
                                                                     
  digitalWrite(dir_pin, LOW);
  if (customSpeedMapped>200) { // Mode basse vitesse. Ici l'accélération et la décélération ne sont pas requise.

     while(steps<customCourseMapped){  // va jusqu'à course
       digitalWrite(step_pin, HIGH);
       delayMicroseconds(customSpeedMapped);          
       digitalWrite(step_pin, LOW); 
       delayMicroseconds(customSpeedMapped);
       steps++;
       //Serial.println(steps);
       // We only want to read the pot every so often (because it takes a long time we don't
       // want to do it every time through the main loop).  
       if (analog_read_counter > 0) {
         analog_read_counter--;
       }
       else {
         analog_read_counter = 300;
         customSpeedMapped =speedUp(); // permets de modifier la vitesse sans devoir attendre un va complet
       }
     }
    
     digitalWrite(dir_pin, HIGH); //changement de direction et va jusqu'à la référence 0
     while(steps>ref){
       digitalWrite(step_pin, HIGH);
       delayMicroseconds(customSpeedMapped);          
       digitalWrite(step_pin, LOW); 
       delayMicroseconds(customSpeedMapped);
       steps--;
       //Serial.println(steps);
       // We only want to read the pot every so often (because it takes a long time we don't
       // want to do it every time through the main loop).  
       if (analog_read_counter > 0) {
         analog_read_counter--;
       }
       else {
         analog_read_counter = 300;
         customSpeedMapped =speedUp(); // permets de modifier la vitesse sans devoir attendre un va complet
       }
     }
  }

  else{ 
    
  // ici la vitesse rapide est activée et l'accélération et la décélértion est requise
    
  digitalWrite(dir_pin, LOW); 

  //accélération et va jusqu'à course
  while(steps<(customCourseMapped-dist_frein)){ // On doit réduire la course pour laisser de la distance au freinage
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x>customSpeedMapped) x--;  // tant que x est plus grand (plus x est grand plus la vitesse est lente) que la vitesse du poto, on accélère
    steps++;
  }
  // Là on a atteint la course - la distance de freinage 
  //décélération
  while (x<v_max_inv){            
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x<v_max_inv) x++;
     steps++; //on doit continuer à incrémenter car ça continue à avancer  
  }
  
  digitalWrite(dir_pin, HIGH); //Changement de direction 
  while(steps>(ref+dist_frein)){ // Acccélération et va jusqu'à la référence + distance de freinage.
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x>customSpeedMapped) x--; 
    steps--; 
  }
  
  while(x<(v_max_inv)){ //décélération
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x<v_max_inv) x++;
    steps--;  
  }
//*/
  }
  //steps=steps+2; //pour compenser je ne sais pas quoi? pour le petit driver il faut mettre +2 et rien pour le grand (commenter)
}

 if(CoteRouge){
  if(customSpeedMapped<v_max_inv){
    dist_frein = v_max_inv-customSpeedMapped;                               // On recalcule la distance de freinage pour la vitesse de consigne actuelle.
    course_min = 380;      //course min plus grande                         // Plus la vitesse est élévée et plus la distance de freinage est longue.
  }                                                                         // Rappel : une valeur petite pour la vitesse signifie une grande vitesse
                                                                            // puisque la valeur est le délai en microsecondes entre-pulse                                                                                           
  else{course_min = 30;} // fonction vibro à basse vitesse
  
  //customCourseMapped = course(); // on lit la course en fonction vibro ou pas*******************************
                                                                     
  digitalWrite(dir_pin, HIGH);
  if (customSpeedMapped>200) { // Mode basse vitesse. Ici l'accélération et la décélération ne sont pas requise.

     while(steps<customCourseMapped){  // va jusqu'à course
      digitalWrite(step_pin, HIGH);
      delayMicroseconds(customSpeedMapped);          
      digitalWrite(step_pin, LOW); 
      delayMicroseconds(customSpeedMapped);
      steps++;
      //Serial.println(steps);
      // We only want to read the pot every so often (because it takes a long time we don't
      // want to do it every time through the main loop).  
      if (analog_read_counter > 0) {
        analog_read_counter--;
      }
      else {
       analog_read_counter = 300;
       customSpeedMapped =speedUp(); // permets de modifier la vitesse sans devoir attendre un va complet
     }
  }
  //*  
  digitalWrite(dir_pin, LOW); //changement de direction et va jusqu'à la référence 0
  while(steps>ref){
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(customSpeedMapped);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(customSpeedMapped);
    steps--;
    //Serial.println(steps);
    // We only want to read the pot every so often (because it takes a long time we don't
    // want to do it every time through the main loop).  
    if (analog_read_counter > 0) {
      analog_read_counter--;
    }
    else {
      analog_read_counter = 300;
      customSpeedMapped =speedUp(); // permets de modifier la vitesse sans devoir attendre un va complet
    }
  }
}

  else{ 
    
    // ici la vitesse rapide activée et l'accélération et la décélértion est requise
    
  digitalWrite(dir_pin, HIGH); 

  //accélération et va jusqu'à course
  while(steps<(customCourseMapped-dist_frein)){ // On doit réduire la course pour laisser de la distance au freinage
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x>customSpeedMapped) x--;  // tant que x est plus grand (plus x est grand plus la vitesse est lente) que la vitesse du poto, on accélère
    steps++;
  }
  // Là on a atteint la course - la distance de freinage 
  //décélération
  while (x<v_max_inv){            
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x<v_max_inv) x++;
     steps++; //on doit continuer à incrémenter car ça continue à avancer  
  }
  
  digitalWrite(dir_pin, LOW); //Changement de direction 
  while(steps>(ref+dist_frein)){ // Acccélération et va jusqu'à la référence + distance de freinage.
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x>customSpeedMapped) x--; 
    steps--; 
  }
  
  while(x<(v_max_inv)){ //décélération
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x<v_max_inv) x++;
    steps--;  
  }
//*/
  }
  //steps=steps+2; //pour compenser je ne sais pas quoi? pour le petit driver il faut mettre +2 et rien pour le grand (commenter)
}
if(CoteMilieu){
  if(customSpeedMapped<v_max_inv){
    dist_frein = v_max_inv-customSpeedMapped;                               // On recalcule la distance de freinage pour la vitesse de consigne actuelle.
    course_min = 380;      //course min plus grande                         // Plus la vitesse est élévée et plus la distance de freinage est longue.
  }                                                                         // Rappel : une valeur petite pour la vitesse signifie une grande vitesse
                                                                            // puisque la valeur est le délai en microsecondes entre-pulse                                                                                           
  else{course_min = 30;} // fonction vibro à basse vitesse

 //customCourseMapped = course(); // on lit la course en fonction vibro ou pas**********************
  
                                                                     
  digitalWrite(dir_pin, LOW);
  if (customSpeedMapped>200) { // Mode basse vitesse. Ici l'accélération et la décélération ne sont pas requise.

     while(steps<((course_max/2)+(customCourseMapped)/2)){  // va jusqu'à course
      digitalWrite(step_pin, HIGH);
      delayMicroseconds(customSpeedMapped);          
      digitalWrite(step_pin, LOW); 
      delayMicroseconds(customSpeedMapped);
      steps++;
      //Serial.println(steps);
      // We only want to read the pot every so often (because it takes a long time we don't
      // want to do it every time through the main loop).  
      if (analog_read_counter > 0) {
        analog_read_counter--;
      }
      else {
       analog_read_counter = 300;
       customSpeedMapped =speedUp(); // permets de modifier la vitesse sans devoir attendre un va complet
     }
  }
  //*  
  digitalWrite(dir_pin, HIGH); //changement de direction et va jusqu'à la référence 0
  while(steps>((course_max/2)-(customCourseMapped/2))){
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(customSpeedMapped);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(customSpeedMapped);
    steps--;
    //Serial.println(steps);
    // We only want to read the pot every so often (because it takes a long time we don't
    // want to do it every time through the main loop).  
    if (analog_read_counter > 0) {
      analog_read_counter--;
    }
    else {
      analog_read_counter = 300;
      customSpeedMapped =speedUp(); // permets de modifier la vitesse sans devoir attendre un va complet
    }
  }
}

  else{ 
    
    // ici la vitesse rapide activée et l'accélération et la décélértion est requise
    
  digitalWrite(dir_pin, LOW); 

  //accélération et va jusqu'à course
  while(steps<((course_max/2)+(customCourseMapped/2)-dist_frein)){ // On doit réduire la course pour laisser de la distance au freinage steps<((course_max/2)+(customCourseMapped/2)-dist_frein)
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x>customSpeedMapped) x--;  // tant que x est plus grand (plus x est grand plus la vitesse est lente) que la vitesse du poto, on accélère
    steps++;
  }
  // Là on a atteint la course - la distance de freinage 
  //décélération
  while (x<v_max_inv){            
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x<v_max_inv) x++;
     steps++; //on doit continuer à incrémenter car ça continue à avancer  
  }
  
  digitalWrite(dir_pin, HIGH); //Changement de direction 
  while(steps>((course_max/2)-(customCourseMapped/2)+dist_frein)){ // Acccélération et va jusqu'à la référence + distance de freinage. steps>((course_max/2)-(customCourseMapped/2))
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x>customSpeedMapped) x--; 
    steps--; 
  }
  
  while(x<(v_max_inv)){ //décélération
    digitalWrite(step_pin, HIGH);
    delayMicroseconds(x);          
    digitalWrite(step_pin, LOW); 
    delayMicroseconds(x);
    //Serial.println(steps);
    if(x<v_max_inv) x++;
    steps--;  
  }
//*/
  }
  //steps=steps+2; //pour compenser je ne sais pas quoi? pour le petit driver il faut mettre +2 et rien pour le grand (commenter)
}
customCourseMapped_pre = customCourseMapped; //pour la calcul de la diff (sécurité)
}
  // Function for reading the Potentiometer
    int speedUp() {
      int customSpeed = analogRead(A0); // Reads the potentiometer
      int newCustom = map(customSpeed, 1023, 0, v_max,v_min);
      return newCustom;  
   }
   
    int course() {
      int customCourse = analogRead(A1); // Reads the potentiometer
      int newCustom2 = map(customCourse, 1023, 0, course_max,course_min);
      return newCustom2;
    } 
