// CodeGeckoAccelStepV1.ino
// V1: Version de base
// Auteur: LM42P
// Date: 4 july 2022
// Driver used: Geckodrive G201X
// Motor: Stepper Motor NEMA 23 23HS11240
// Arduino: Arduino Uno
//
// Ce programme utilise la librairie AccelStepper pour un changement de sens plus doux. Il supprime
// le saut de vitesse par rapport au CodeGeckoVx.ino, au détriment du vitesse plus faible.

#include <AccelStepper.h>           // Load the AccelStepper library
  
// direction Digital 8 (CCW), pulses Digital 9 (CLK)
AccelStepper stepper(1, 9, 8); 

  //****************************************************************************************************
  //**********************PARAMETRES MODIFIABLES********************************************************
  
  #define v_max 5000        // C'est la vitesse maximale lue par le potentiomètre de vitesse. Peut être
                            // descendue à 4800, car il y un bout vers la fin qui ne va pas plus vite.
  #define v_min 120         // C'est la vitesse minimum lue par le potentiomètre de vitesse.
  #define course_max_physique 3044 // C'est la course maximale physique de la machine (butée à butée 
                            // sans jeu). Cette valeur a été détérminée par la programme setFullStroke.
  #define course_max course_max_physique-2*marge // C'est la valeur de la course effective (tenant 
                            // compte des marges. Cette valeur sert à mapper le potentiomètre 
                            // de course.

//       |<------------------------------------course_max_physique------------------------------->|
//       |<--marge-->|<-----------------------------course_max----------------------->|<--marge-->|
//                   |<-->|                                                      |<-->|
//                 course_min                                                  course_min

//       |<--marge-->|<---------course_max/2--------->|<---------course_max/2-------->|<--marge-->|     

//                                                |<->|<->|
//                                        course_min/2 course_min/2
                  
  #define course_min 40     // c'est la course minimum pour que le vibro soit efficace. Une valeur trop
                            // petite donnera des amplitudes de variation trop petites.                     
  #define marge 15          // C'est la marge ou décalage de la référence par rapport à la position du 
                            // bras à la mise sous tension de la machine en butée soit du coté vert ou
                            // rouge. Ceci assure que la partie mobile ne viennent cogné sur l'un des
                            // cotés. J'ai baissé cette valeur de 30 à 15, pour profiter de la course
                            // au max.
 
 
 // Points définissant les plages sur les potentiomètres (voir schéma):

  #define i 0.2 // facteur de dimensionnement de la plage
  #define A_course course_min // on met une marge de 10 pour être sûr que l'on soit dans la plage
                              // en butée
  #define B_course A_course+plage_course
  #define C_course (F_course-A_course)/2+A_course-plage_course/2
  #define D_course (F_course-A_course)/2+A_course+plage_course/2 
  #define E_course F_course-plage_course
  #define F_course course_max // on met une marge de 10 pour être sûr que l'on soit dans la plage
                              // en butée                           
  #define plage_course i*(F_course-A_course)

  bool plage_min_course=false;
  bool plage_milieu_course=false;
  bool plage_max_course=false;
  
  #define A_vitesse v_min // on met une marge de 10 pour être sûr que l'on soit dans la plage
                             // en butée
  #define B_vitesse A_vitesse+plage_vitesse
  #define C_vitesse (F_vitesse-A_vitesse)/2+A_vitesse-plage_vitesse/2
  #define D_vitesse (F_vitesse-A_vitesse)/2+A_vitesse+plage_vitesse/2 
  #define E_vitesse F_vitesse-plage_vitesse
  #define F_vitesse v_max // on met une marge de 10 pour être sûr que l'on soit dans la plage
                             // en butée                            
  #define plage_vitesse i*(F_vitesse-A_vitesse)

  bool plage_min_vitesse=false;
  bool plage_milieu_vitesse=false;
  bool plage_max_vitesse=false;

  bool CoteVert = false;   // Pour définir le côté vert avec les potos
  bool CoteRouge = false;  // Pour définir le côté rouge avec les potos
  bool Milieu = false;     // Pour définir l'utilisation à deux de la machine
                           // (les bras se mettent au milieu)

  int cours=0; // C'est la course
  int customSpeedMapped = 5;         // Pour stocker la consigne de la vitesse
  int analog_read_counter = 300;     // Compteur est utilisé pour ne de pas lire la vitesse du potentiometre
                                     // trop souvent. Ce qui permet une vitesse plus élevée du va-et-vient
                                     // et évite les parasites.                           

void setup() {
  delay(500);                        // on attend un peu pour "déparasiter" après branchement.
  stepper.setMaxSpeed(500.0);        // set the max motor speed 
  stepper.setAcceleration(300000.0); // set the acceleration 3000000 is the max avant 
                                      // 3000000 (max): putain de vibro!!! mais pour dildo légé max 800g 
                                      // 300000: léger décrochage avec 2.5kg mais vibro pas mal
                                      // 100000: plus aucun décrochage avec 2.5kg, mais vibro médiocre

//*******************************************************************************************
  // Ici on vérifie la position des potensiomètres pour savoir de quel côté la machine
  // est utilisée. (voir schéma)
  //*****************************************************************************************

//Le curseur de potentiomètre de la course est dans la plage min
  if (course()>=A_course-10 && course()<B_course){
    plage_min_course=true;
    plage_milieu_course=false;
    plage_max_course=false;
  }

//Le curseur de potentiomètre de la course est dans la plage milieu
  else if (course()>C_course && course()<D_course){
    plage_min_course=false;
    plage_milieu_course=true;
    plage_max_course=false;
  }

//Le curseur de potentiomètre de la course est dans la plage max
  else if (course()>E_course && course()<=F_course+10){ //+10 plage plus grande
    plage_min_course=false;
    plage_milieu_course=false;
    plage_max_course=true;
  }
  else {
    plage_min_course=false;
    plage_milieu_course=false;
    plage_max_course=false;
  }

//Le curseur de potentiomètre de la vitesse est dans la plage min
  if (speedUp()>=A_vitesse-10 && speedUp()<B_vitesse){ //-10 plage plus grande
    plage_min_vitesse=true;
    plage_milieu_vitesse=false;
    plage_max_vitesse=false;
  }

//Le curseur de potentiomètre de la vitesse est dans la plage milieu
  else if (speedUp()>C_vitesse && speedUp()<D_vitesse){
    plage_min_vitesse=false;
    plage_milieu_vitesse=true;
    plage_max_vitesse=false;
  }
  
//Le curseur de potentiomètre de la vitesse est dans la plage max
  else if (speedUp()>E_vitesse && speedUp()<=F_vitesse+10){ //+10 plage plus grande
    plage_min_vitesse=false;
    plage_milieu_vitesse=false;
    plage_max_vitesse=true;
  }
  else {
    plage_min_vitesse=false;
    plage_milieu_vitesse=false;
    plage_max_vitesse=false;
    }

  //ici la machine est utilisée du côté vert
  
  if(plage_min_course && plage_min_vitesse){
    CoteVert = true;
    Milieu = false; 
    CoteRouge = false;
  }

  //ici la machine est utilisée sur les deux côtés à la fois
  
  else if(plage_milieu_course && plage_milieu_vitesse){
    CoteVert = false;
    Milieu = true; 
    CoteRouge = false;
  }
 
  
  //ici la machine est utilisée du côté rouge
  
  else if(plage_max_course && plage_max_vitesse){
    CoteVert = false;
    Milieu = false; 
    CoteRouge = true;
  }

  else{
    CoteVert = false;
    Milieu = false; 
    CoteRouge = false;
    } 

// on met les bonnes références suivant le côté voulu

  //ici la machine est utilisée du côté vert 
  if(CoteVert){

    // On référence la position à marge (pour laisser de la place à la butée)
    stepper.setCurrentPosition(marge); 

    // On se déplace à 0.
    stepper.moveTo(0); 
    while (!stepper.distanceToGo() == 0) {
    stepper.run();
    } 
  }

//ici la machine est utilisée sur les deux côtés à la fois (potentiomètres au milieu)

  else if(Milieu){
    // On référence la position à marge (pour laisser de la place à la butée)
    stepper.setCurrentPosition(course_max_physique/2); 

    // On se déplace à 0
    stepper.moveTo(0); 
    while (!stepper.distanceToGo() == 0) {
    stepper.run();
    } 
  }
  
//ici la machine est utilisée du côté rouge
  else if(CoteRouge){
    //On reset la position à -marge (pour laisser de la place à la butée)
    stepper.setCurrentPosition(-marge); 
                                      
    // On se déplace à 0.
    stepper.moveTo(0); 
    while (!stepper.distanceToGo() == 0) {
      stepper.run();
    }
  }

  // Tant que le potentiomètre de la vitesse et de la course ne sont pas au minimum, on ne 
  // démarre pas (pour la sécurité)
  while (!(speedUp() < (v_min + 30) && course() < course_min+100)){}
  
  } //fin du setup

void loop() {
  
//*********si le potentiomètre de la vitesse est au minimum, on met LM42P à l'arrêt**************
  while ((customSpeedMapped) < (v_min +7)){
                                            
    if (analog_read_counter > 0) { //pour ne pas lire trop souvent le pot de la vitesse
      analog_read_counter--;
    }
    else {
      analog_read_counter = 300;
      customSpeedMapped =speedUp();
    }
  } // fin de la boucle while //mettre la machine à l'arrêt

  // on règle la vitesse de la machine
  if (analog_read_counter > 0) { //pour ne pas lire le pot de la vitesse trop souvent et évite les parasites
      analog_read_counter--;
  }
  else {
    analog_read_counter = 300;
    customSpeedMapped =speedUp(); 
    stepper.setMaxSpeed(customSpeedMapped); // permet de modifier la vitesse sans devoir attendre
                                            // un va-et-vient complet
  }  // fin on règle la vitesse de la machine
  
  if(CoteVert){ // si la machine est utilisée du côté vert
  // If the stepper isn't moving and doesn't have to go any distance
    if (!stepper.isRunning() && stepper.distanceToGo() == 0) {
      stepper.moveTo(-cours); 
      if (cours>0){
        cours=0;
      } 
      else cours=course();
    }
  }

  else if(Milieu){ // si la machine est utilisée des deux côtés à la fois
  // If the stepper isn't moving and doesn't have to go any distance
    if (!stepper.isRunning() && stepper.distanceToGo() == 0) {
      stepper.moveTo(cours);
      cours=(-1*cours);
      if (cours>=0){
        cours=course()/2;
      }
    }
  }
  
  else if(CoteRouge){ // si la machine est utilisée du côté rouge
  // If the stepper isn't moving and doesn't have to go any distance
    if (!stepper.isRunning() && stepper.distanceToGo() == 0) {
      stepper.moveTo(cours);
      if (cours>0){
        cours=0;
      } 
      else cours=course();
    }
  }

  stepper.run();

} //fin de la boucle principale

  // Function for reading the Potentiometer
    int speedUp() {
      int customSpeed = analogRead(A0); // Reads the potentiometer
      int newCustom = map(customSpeed, 0, 1023, v_min,v_max);
      return newCustom;  
   }
   
    int course() {
      int customCourse = analogRead(A1); // Reads the potentiometer
      int newCustom2 = map(customCourse, 0, 1023, course_min, course_max);
      return newCustom2;
    }
