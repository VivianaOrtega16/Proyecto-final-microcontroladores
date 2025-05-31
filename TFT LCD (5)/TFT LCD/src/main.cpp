
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Sprite.h"
#include "Cactus.h"
#include "Moneda.h"
#include "fondo.h"
#include "gameoverwin.h"

#define TFT_DC 7
#define TFT_CS 6
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 10
#define TFT_MISO 12
#define BUZZER_PIN 15
#define botonRight 18

const int XMAX = 240;
const int YMAX = 320;

// Clase para manejar el display
class Display { 
    // esta clase encapsula todo lo que se puede hacer en la pantalla
    // sin tner que escribir muchas veces el mismo codigo , solop llamo funciones 
private:
    Adafruit_ILI9341 screen;// es un atributo, una isntancia de la libreria adafruit 
    // que es la que nos ayuda a graficar todo;
    
public:
    // constructoir de la clase 
    Display() : screen(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO) {}
    
    void init() {
        screen.begin(); // este metodo se usa oara inicual la pantalla
    }
    
    void fillScreen(uint16_t color) {
        screen.fillScreen(color);// llena la pantalla de color , lo bueno es que si 
        // llamamos este metodo podemo susar cualquier color
    }
    
    void fillRect(int x, int y, int w, int h, uint16_t color) {
        screen.fillRect(x, y, w, h, color);
    }// dibuja un rectangulo de color lo usamos para eliminar la posicion anmterior del dino
    
    void drawRGBBitmap(int x, int y, const uint16_t* bitmap, int w, int h) {
        screen.drawRGBBitmap(x, y, bitmap, w, h);
    }// esta dibuja un sprite con una posicion , y un tamaño de pixeles 
    
    void drawLine(int x0, int y0, int x1, int y1, uint16_t color) {
        screen.drawLine(x0, y0, x1, y1, color);
    }// dibuja una linea entre dos puntos util para el piso
    
    // estas funciones son para escribir los textos 
    void setTextColor(uint16_t color) {
        screen.setTextColor(color);
    }
    
    void setTextSize(uint8_t size) {
        screen.setTextSize(size);
    }
    
    void setCursor(int x, int y) {
        screen.setCursor(x, y);
    }
    
    void print(const char* text) {
        screen.print(text);
    }
    
    void print(int value) {
        screen.print(value);
    }
    
    void drawRect(int x, int y, int w, int h, uint16_t color) {
        screen.drawRect(x, y, w, h, color);
    }// dibuja un rectangulo para los HUD
};

// Clase para manejar sonidos
class SoundManager {
private:
    int buzzerPin;// es una propiedad privada que va a guardar el numero del pin donde esta el buzzer 
    // es privado para usarse solo dentro de esta clase
public:
    SoundManager(int pin) : buzzerPin(pin) {}// constructor de la clase
    
    void playCollisionSound() {//choco contra el cactus
        tone(buzzerPin, 300, 200); //pin, Hz, milisegundos
    }
    
    void playCoinSound() {
        tone(buzzerPin, 1000, 150);
    }
    
    void playGameOverSound() {
        tone(buzzerPin, 500, 800);
        delay(100);
        tone(buzzerPin, 250, 800);
        // se ejecutan dos tonos 
    }
    
    void playVictorySound() {
        for (int freq = 400; freq <= 1000; freq += 100) {
            tone(buzzerPin, freq, 80);
            delay(100);
        }
        noTone(buzzerPin);
    }
    
    void stopSound() {
        noTone(buzzerPin);//detiene cualquier sonido
    }
};

// Clase base para objetos del juego
class GameObject {// esta es una clase abstracta porwue no crea objetos directamente si no ss clases hijas
protected:// solo pueden ser accesibles desde clases hijas
    int x, y;
    int width, height;
    
public:
    GameObject(int startX, int startY, int w = 32, int h = 32) // valores por defecto ya que la mayorita, el dino , el cactus y la moneda son de estas medidas
        : x(startX), y(startY), width(w), height(h) {} // constructor de la clase
    
    virtual ~GameObject() {} // Destructor virtual
    
    virtual void draw(Display& display) = 0;
    virtual void update() {}
    
    int getX() const { return x; }
    int getY() const { return y; }
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    //estos son metodos para acceder a las propiedades del objeto de fornma segura
    // las cuales las bcesitamos para verificar objetos y colisiones 
    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
        //permite al objeto mover a nuevas coordenadas 
    }
    
    bool checkCollision(const GameObject& other) const {
        return (x + width > other.x && x < other.x + other.width &&
                y + height > other.y && y < other.y + other.height);
    }// este metodo lo que va es a verificar si dos objetos estan en el mismo espacio sea el dino con 
    // cactus o el dino con monedas
    
};

// Clase para el jugador (Dino)
class DinoPlayer : public GameObject {
    //crea una clase hija que hereda todos los metdodos de GameObject
    // puede usar o sobreescribir los metodos 
private:
    // tiene sus propios atributos privados solo accesibles dentro de la clase 
    int lastX, lastY; // guardan la posicion anterior del jugador
    // para qeu se pueda borrar la posicion anterior 
    int frame; // el dino
    bool isJumping;
    int jumpHeight;
    int fallSpeed;// velocidad con la que cae despues de saltar
    int currentLevel;
    int* floorLevels;// puntero al array con las alturas de los pisos 
    Display* display;//puntero al display para dibujar o borrar los sprites 
    
public:
    DinoPlayer(int startX, int startY, int* floors, Display* disp) 
        : GameObject(startX, startY), lastX(startX), lastY(startY), 
          frame(0), isJumping(false), jumpHeight(58), fallSpeed(13),
          currentLevel(0), floorLevels(floors), display(disp) {}// tiene su propio constructor
    
    void draw(Display& display) override { //sobreescribe dek metodo draw de l a clase padre
        // Borrar posición anterior
        display.fillRect(lastX, lastY, width, height, ILI9341_BLACK);
        
        // Dibujar en nueva posición
        //los :: significa que Player esta en un mabito global , fuera 
        // de cualquier espacio o clase 
        display.drawRGBBitmap(x, y, ::Player[frame], width, height);
        
        // Actualizar última posición
        lastX = x;
        lastY = y;
    }
    
    void update() override {
        // Actualizar animación
        frame = (frame + 1) % 2;// va a cmabiar entre los dos sprites del dino para que 
        // se vea el movimineto de las patitas
        
        // Manejar física de salto
        if (isJumping && y < floorLevels[currentLevel] - height) {
            display->fillRect(x, y, width, height, ILI9341_BLACK);
            y += fallSpeed;// si esta saltando y no ha llegado al piso lo hace caer
            lastY = y;
        } else {
            isJumping = false;// si toco el suelo temrina el salto , por lo que cambia su estado 
            y = floorLevels[currentLevel] - height;// ajista la posiciom exacta ak salto
        }
    }
    
    void moveRight() {
        x += 10; // se movera solo 10 px esto lo puedo ajustar
    }
    
    void jump() {
        // si no esta saltando inicia el salto
        if (!isJumping) {
            isJumping = true;
            display->fillRect(x, y, width, height, ILI9341_BLACK);// lo borra de  la pantalla
            y -= jumpHeight;
            x += 50;
            lastY = y;
            lastX = x;
        }
    }
    
    void resetPosition() {
        x = 0;
        y = floorLevels[currentLevel] - height;// en el suelo del nivek que estaba 
        isJumping = false;
    }
    // aceder a las propiedades privadas de la clase padrepara cambiar el nivel , retornat el nivel
    // y que continue al siguiente nivel si llego a XMAX
    void setCurrentLevel(int level) {
        currentLevel = level;
    }
    
    int getCurrentLevel() const {
        return currentLevel;
    }
    
    bool isAtRightEdge() const {
        return x >= XMAX - width;
    }
    
    void clearFromScreen() {
        display->fillRect(x, y, width, height, ILI9341_BLACK);
    }// borra al jugador de la posicion actual para que pase al sgt nivel
};

// Clase para obstáculos (Cactus)
class Obstacle : public GameObject {
private:
    bool isActive;
    
public:
    Obstacle(int startX, int startY) : GameObject(startX, startY), isActive(true) {}
    
    virtual ~Obstacle() {} // Destructor virtual
    
    void draw(Display& display) override {
        if (isActive) {
            display.drawRGBBitmap(x, y, spriteCactus, width, height);
        }
    }
    
    bool isActiveObstacle() const {
        return isActive;
    }
    
    void deactivate() {
        isActive = false;
    }
};

// Clase para monedas
class Coin : public GameObject {
private:
    bool isCollected;
    
public:
    Coin(int startX, int startY) : GameObject(startX, startY), isCollected(false) {}
    
    virtual ~Coin() {} // Destructor virtual
    
    void draw(Display& display) override {
        if (!isCollected) {
            display.drawRGBBitmap(x, y, spriteMoneda, width, height);
        }
    }
    
    void collect(Display& display) {
        if (!isCollected) {
            isCollected = true;
            display.fillRect(x, y, width, height, ILI9341_BLACK);
            x = -100; // Mover fuera de pantalla
        }
    }
    
    bool isCollectedCoin() const {
        return isCollected;
    }
};

// Clase para niveles
class Level {
private:
    int levelNumber;
    int speed;
    int cactusCount;
    int coinCount;
    Obstacle** obstacles;// Como son varios entonces es un array de punteros a la clase obstacle
    // es un arreglo dinamico, es decir no esta predeterminado si noq ue cuando se inicia el juego se determian
    //porque en la clase level se define cuenatos quiero y luego se crea el arrgelo dinamico
    // entonces con el bucle los crea y los va guardando en ese arreglo
    // el constructor de la clase decidee cuantos cactus se van a crear 
    Coin** coins;
    
public:
    Level(int number, int spd, int cactusNum, int coinNum, 
          Obstacle** obs, Coin** cn) 
        : levelNumber(number), speed(spd), cactusCount(cactusNum), 
          coinCount(coinNum), obstacles(obs), coins(cn) {}
    
    void drawObstacles(Display& display) {
        for (int i = 0; i < cactusCount; i++) {
            obstacles[i]->draw(display);
        }
    }
    
    void drawCoins(Display& display) {
        for (int i = 0; i < coinCount; i++) {
            coins[i]->draw(display);
        }
    }
    
    Obstacle** getObstacles() { return obstacles; }
    Coin** getCoins() { return coins; }
    int getCactusCount() const { return cactusCount; }
    int getCoinCount() const { return coinCount; }
    int getSpeed() const { return speed; }
};

// Clase principal del juego
class Game {
private:
    Display display;
    SoundManager soundManager;
    DinoPlayer* player;
    Level** levels;
    int currentLevel;
    int lives;
    int score;
    int floorLevels[4];
    bool gameRunning;
    
    // Obstáculos y monedas para cada nivel
    Obstacle* cactusLevel1[2];
    Obstacle* cactusLevel2[3];
    Obstacle* cactusLevel3[4];
    
    Coin* coinsLevel1[2];
    Coin* coinsLevel2[3];
    Coin* coinsLevel3[4];
    
public:
    Game() : soundManager(BUZZER_PIN), currentLevel(0), lives(3), score(0), gameRunning(true) {
        // Inicializar niveles del piso
        floorLevels[0] = YMAX - 60;
        floorLevels[1] = YMAX - 160;
        floorLevels[2] = YMAX - 260;
        floorLevels[3] = YMAX - 32;
        
        // Crear jugador
        player = new DinoPlayer(0, floorLevels[0] - 32, floorLevels, &display);
        
        initializeGameObjects();
        initializeLevels();
    }
    
    ~Game() {
        delete player;
        for (int i = 0; i < 3; i++) {
            delete levels[i];
        }
        delete[] levels;
        
        // Limpiar obstáculos y monedas
        for (int i = 0; i < 2; i++) {
            delete cactusLevel1[i];
            delete coinsLevel1[i];
        }
        for (int i = 0; i < 3; i++) {
            delete cactusLevel2[i];
            delete coinsLevel2[i];
        }
        for (int i = 0; i < 4; i++) {
            delete cactusLevel3[i];
            delete coinsLevel3[i];
        }
    }
    
    void initializeGameObjects() {
        // Crear obstáculos nivel 1
        cactusLevel1[0] = new Obstacle(100, floorLevels[0] - 32);
        cactusLevel1[1] = new Obstacle(200, floorLevels[0] - 32);
        
        // Crear obstáculos nivel 2
        cactusLevel2[0] = new Obstacle(30, floorLevels[1] - 32);
        cactusLevel2[1] = new Obstacle(170, floorLevels[1] - 32);
        cactusLevel2[2] = new Obstacle(200, floorLevels[1] - 32);
        
        // Crear obstáculos nivel 3
        cactusLevel3[0] = new Obstacle(30, floorLevels[2] - 32);
        cactusLevel3[1] = new Obstacle(170, floorLevels[2] - 32);
        cactusLevel3[2] = new Obstacle(200, floorLevels[2] - 32);
        cactusLevel3[3] = new Obstacle(220, floorLevels[2] - 32);
        
        // Crear monedas nivel 1
        coinsLevel1[0] = new Coin(80, floorLevels[0] - 32);
        coinsLevel1[1] = new Coin(180, floorLevels[0] - 32);
        
        // Crear monedas nivel 2
        coinsLevel2[0] = new Coin(60, floorLevels[1] - 32);
        coinsLevel2[1] = new Coin(140, floorLevels[1] - 32);
        coinsLevel2[2] = new Coin(210, floorLevels[1] - 32);
        
        // Crear monedas nivel 3
        coinsLevel3[0] = new Coin(70, floorLevels[2] - 32);
        coinsLevel3[1] = new Coin(130, floorLevels[2] - 32);
        coinsLevel3[2] = new Coin(190, floorLevels[2] - 32);
        coinsLevel3[3] = new Coin(220, floorLevels[2] - 32);
    }
    
    void initializeLevels() {
        levels = new Level*[3];
        levels[0] = new Level(1, 2, 2, 2, (Obstacle**)cactusLevel1, (Coin**)coinsLevel1);
        levels[1] = new Level(2, 3, 3, 3, (Obstacle**)cactusLevel2, (Coin**)coinsLevel2);
        levels[2] = new Level(3, 4, 4, 4, (Obstacle**)cactusLevel3, (Coin**)coinsLevel3);
    }
    
    void init() {
        Serial.begin(9600);
        Serial.println("Serial inicializado");
        
        attachInterrupt(digitalPinToInterrupt(botonRight), []() {
            // Callback para salto - se maneja en el loop principal
        }, RISING);
        
        display.init();
        showStartScreen();
        
        display.fillScreen(ILI9341_BLACK);
        drawFloor();
        drawAllObstacles();
        
        sei();
    }
    
    void showStartScreen() {
        display.fillScreen(ILI9341_WHITE);
        
        display.setTextColor(ILI9341_RED);
        display.setTextSize(6);
        display.setCursor(XMAX / 2 - 80, 70);
        display.print("DINO");
        
        display.drawRGBBitmap(XMAX / 2 - 32, 130, spriteFondo, 64, 114);
        
        delay(3000);
    }
    
    void drawFloor() {
        display.setTextColor(ILI9341_WHITE);
        display.setTextSize(2);
        
        for (int i = 0; i < 4; i++) {
            display.drawLine(0, floorLevels[i], XMAX, floorLevels[i], ILI9341_WHITE);
        }
    }
    
    void drawHUD() {
        display.fillRect(0, YMAX - 20, XMAX, 16, 0x03E0);
        display.setTextColor(ILI9341_WHITE);
        display.setTextSize(1);
        
        display.setCursor(5, YMAX - 18);
        display.print("Vidas: ");
        for (int i = 0; i < 3; i++) {
            if (i < lives) {
                display.fillRect(60 + i * 12, YMAX - 18, 10, 10, ILI9341_RED);
            } else {
                display.drawRect(60 + i * 12, YMAX - 18, 10, 10, ILI9341_WHITE);
            }
        }
        
        display.setCursor(140, YMAX - 18);
        display.print("Puntos: ");
        display.print(score);
    }
    
    void drawAllObstacles() {
        levels[0]->drawObstacles(display);
        levels[0]->drawCoins(display);
        levels[1]->drawObstacles(display);
        levels[1]->drawCoins(display);
        levels[2]->drawObstacles(display);
        levels[2]->drawCoins(display);
    }
    
    void checkCollisions() {
        Level* level = levels[currentLevel];
        
        // Verificar colisiones con obstáculos
        Obstacle** obstacles = level->getObstacles();
        for (int i = 0; i < level->getCactusCount(); i++) {
            if (obstacles[i]->isActiveObstacle() && player->checkCollision(*obstacles[i])) {
                soundManager.playCollisionSound();
                lives--;
                player->resetPosition();
                break;
            }
        }
        
        // Verificar colisiones con monedas
        Coin** coins = level->getCoins();
        for (int i = 0; i < level->getCoinCount(); i++) {
            if (!coins[i]->isCollectedCoin() && player->checkCollision(*coins[i])) {
                soundManager.playCoinSound();
                score += 10;
                coins[i]->collect(display);
            }
        }
        
        // Verificar condiciones de fin de juego
        if (score >= 80) {
            showVictoryScreen();
            gameRunning = false;
        }
        
        if (lives <= 0) {
            showGameOverScreen();
            gameRunning = false;
        }
    }
    
    void checkLevelProgression() {
        if (player->isAtRightEdge()) {
            if (currentLevel < 2) {
                player->clearFromScreen();
                currentLevel++;
                player->setCurrentLevel(currentLevel);
                player->setPosition(0, floorLevels[currentLevel] - 32);
            }
        }
    }
    
    void showVictoryScreen() {
        display.fillScreen(ILI9341_BLACK);
        soundManager.playVictorySound();
        
        display.setTextColor(ILI9341_GREEN);
        display.setTextSize(5);
        
        display.setCursor((XMAX - (3 * 30)) / 2, YMAX / 2 - 50);
        display.print("YOU");
        
        display.setCursor((XMAX - (3 * 30)) / 2, YMAX / 2);
        display.print("WIN");
        
        display.drawRGBBitmap(XMAX / 2 - 32, YMAX / 2 + 60, spriteGameover, 64, 114);
        
        delay(3000);
    }
    
    void showGameOverScreen() {
        display.fillScreen(ILI9341_BLACK);
        soundManager.playGameOverSound();
        
        display.setTextColor(ILI9341_RED);
        display.setTextSize(5);
        int textWidth = 60;
        
        display.setCursor(XMAX / 2 - textWidth, YMAX / 2 - 50);
        display.print("GAME");
        
        display.setCursor(XMAX / 2 - textWidth, YMAX / 2);
        display.print("OVER");
        
        display.drawRGBBitmap(XMAX / 2 - 32, YMAX / 2 + 60, spriteGameover, 64, 114);
        
        delay(3000);
    }
    
    void handleInput() {
        // Aquí manejarías la entrada del botón de salto
        // En el código original usas interrupciones, puedes mantener esa lógica
        if (digitalRead(botonRight) == HIGH) {
            player->jump();
        }
    }
    
    void update() {
        if (!gameRunning) {
            return;
        }
        
        // Limpiar posición anterior del jugador
        display.fillRect(player->getX(), player->getY(), 32, 32, ILI9341_BLACK);
        
        // Actualizar jugador
        player->update();
        player->moveRight();
        
        // Dibujar todo
        player->draw(display);
        drawFloor();
        drawHUD();
        
        // Verificar colisiones y progresión de nivel
        checkCollisions();
        checkLevelProgression();
        
        // Delay basado en la velocidad del nivel
        delay(100 - levels[currentLevel]->getSpeed() * 10);
    }
    
    bool isRunning() const {
        return gameRunning;
    }
};

// Variables globales
Game* game;

void jumpInterrupt() {
    if (game) {
        // Aquí puedes acceder al jugador a través del juego si lo haces público
        // o crear un método público en Game para manejar el salto
    }
}

void setup() {
    game = new Game();
    game->init();
}

void loop() {
    if (game->isRunning()) {
        game->handleInput(); // <-- Agrega esta línea
        game->update();
    }
}





//------------------------------------------------------------------------------------------------------------------------------





//codigo de juego en c normal anterior



/*

#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Sprite.h"
#include "Cactus.h"
#include "Moneda.h"
#include "fondo.h"
#include  "gameoverwin.h"

#define TFT_DC 7
#define TFT_CS 6
#define TFT_MOSI 11
#define TFT_CLK 13
#define TFT_RST 10
#define TFT_MISO 12
#define BUZZER_PIN 15 // Pin del buzzer


#define botonRight 18

const int XMAX = 240;
const int YMAX = 320;
int x = 0;
int y = YMAX - 32; // Posición inicial del dino
int lastX = x, lastY = y; // Variables para rastrear la última posición DEL DINO
int pisoNiveles[] = { YMAX - 60, YMAX - 160, YMAX - 260 };
bool enSalto = false; // Variable para controlar el salto
const int alturaSalto = 58;
const int velocidadCaida = 13;
const uint8_t UP = 0;
const uint8_t DOWN = 1;
const uint8_t LEFT = 3;
const uint8_t RIGHT = 2;//constantes para contrlar la direccion de movimiento
int vidas = 3;
int score = 0;//variables del HUD

Adafruit_ILI9341 screen = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);
//creacion de la pantalla, objeto screen

// Definimos los niveles del juego 
struct Nivel {
    int numero;
    int velocidad;
    int cantidadCactus;
    int cantidadMonedas;
};

Nivel niveles[] = {
    {1, 2, 3, 5},
    {2, 3, 5, 8},
    {3, 4, 7, 12},
    {4, 5, 10, 18}
};

int nivelActual = 0; //guarda en que nivel esta el jugador actualmente 

//  Declaración de funciones 
void setPlayerPosition(int x, int y);
void animatePlayer(int frame);
void moverPlayer(uint8_t direccion);
void moverPlayerIzquierda(void);
void moverPlayerDerecha(void);
void saltar(void);
void verificarNivel(void);
void dibujarPiso(void);
void dibujarObstaculos(void);
void dibujarHUD(void);
void verificarColisiones(void);
void sonidoMoneda(void);
void sonidoCactus(void);
void sonidoGameOver(void);
void mostrarPantallaInicio(void);
void mostrarGameOverDino(void);
void mostrarVictoria(void);

struct Obstaculo {
    int x;
    int y;
};//tanto para monedas y para cactus

// los cactus del nivel 1 igual pero los demas se corren para evitar solapamientos
Obstaculo cactusNivel1[] = {150, pisoNiveles[0] - 32};
Obstaculo cactusNivel2[] = {{30, pisoNiveles[1] - 32}, {170, pisoNiveles[1] - 32}};
Obstaculo cactusNivel3[] = {{30, pisoNiveles[2] - 32}, {170, pisoNiveles[2] - 32}};


Obstaculo monedasNivel1[] = {{80, pisoNiveles[0] - 32}, {180, pisoNiveles[0] - 32}};
Obstaculo monedasNivel2[] = {{60, pisoNiveles[1] - 32}, {140, pisoNiveles[1] - 32}, {210, pisoNiveles[1] - 32}};
Obstaculo monedasNivel3[] = {{70, pisoNiveles[2] - 32}, {130, pisoNiveles[2] - 32}, {190, pisoNiveles[2] - 32}, {220, pisoNiveles[2] - 32}};


//  Configuración inicial 
void setup() {
    Serial.begin(9600); //comunicacion con el puerto serial 
    Serial.println("Serial inicializado");

    //esta es la configuracion para que cuando se presione el boton se llame 
    //a la funcion saltar 
    attachInterrupt(digitalPinToInterrupt(botonRight), saltar, RISING);

    screen.begin();// inicializa la pantalla

    //  Mostrar la pantalla de presentación antes de iniciar el juego
    mostrarPantallaInicio();

    //  Configuración del fondo y elementos del juego
    screen.fillScreen(ILI9341_BLACK);
    dibujarPiso();
    dibujarObstaculos();

    sei();//Habilita interrupciones globales 
}

//  Loop principal del juego 
void loop() {
    static int frame = 0;

    screen.fillRect(x, y, 32, 32, ILI9341_BLACK);//borra al jugador de la posicion anterior

    //-----
    if (vidas <= 0) {
    return; //  Detener loop para que "GAME OVER" se vea correctamente
    }
    //-----

    setPlayerPosition(x, y);// actualiza la posicion del jugador 
    animatePlayer(frame);//dibuja al dino 
    dibujarPiso();
    verificarColisiones(); //tanto con los cactus como con las monedas 
    dibujarHUD();//vidas y puntaje
    moverPlayerDerecha();//para qeu el dino se mueva solito
    verificarNivel();//verifica si cambio de nivel 

    frame = (frame + 1) % 2; //cambia las posiciones del dino 

    if (enSalto && y < pisoNiveles[nivelActual] - 32) {
        screen.fillRect(x, y, 32, 32, ILI9341_BLACK);
        y += velocidadCaida; //simula la caida, "gravedad"
        lastY = y;
    } else {
        enSalto = false;
        y = pisoNiveles[nivelActual] - 32;// si ya no esta cayendo vuelve al piso 
    }

    delay(100 - niveles[nivelActual].velocidad * 10);// aqui puedo editar la velosidad del dino 

    //-------
    //-------
}

//---FUNCIONES DE DIBUJO---

void mostrarPantallaInicio() {
    screen.fillScreen(ILI9341_WHITE); 

    //  Texto grande centrado
    screen.setTextColor(ILI9341_RED);
    screen.setTextSize(6);
    screen.setCursor(XMAX / 2 - 80, 70);
    screen.print("DINO");

    //  Dibujar spriteFondo (ajustado a 64x114) debajo del texto
    screen.drawRGBBitmap(XMAX / 2 - 32, 130, spriteFondo, 64, 114);

    //  Dibujar el sprite del dino centrado debajo de spriteFondo
    //screen.drawRGBBitmap(XMAX / 2 - 16, 250, Player[0], 32, 32);

    delay(3000); //  Espera 3 segundos antes de iniciar el juego
}

void dibujarPiso() {
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(2);

    for (int i = 0; i < 4; i++) {
        screen.drawLine(0, pisoNiveles[i], XMAX, pisoNiveles[i], ILI9341_WHITE);
    }
}

void dibujarHUD() {
    screen.fillRect(0, YMAX - 20, XMAX, 16, 0x03E0);
    screen.setTextColor(ILI9341_WHITE);
    screen.setTextSize(1);

    screen.setCursor(5, YMAX - 18);
    screen.print("Vidas: ");
    for (int i = 0; i < 3; i++) {
        if (i < vidas) {
            screen.fillRect(60 + i * 12, YMAX - 18, 10, 10, ILI9341_RED);
        } else {
            screen.drawRect(60 + i * 12, YMAX - 18, 10, 10, ILI9341_WHITE);
        }
    }

    screen.setCursor(140, YMAX - 18);
    screen.print("Puntos: ");
    screen.print(score);
}



void verificarColisiones() {
    //carga cactus y monedas segun el nivel en el qu esta en dino

    Obstaculo *cactusPos;
    Obstaculo *monedasPos;
    int cantidadCactus, cantidadMonedas;

    if (nivelActual == 0) {
        cactusPos = cactusNivel1;
        monedasPos = monedasNivel1;
        cantidadCactus = 2;
        cantidadMonedas = 2;
    } else if (nivelActual == 1) {
        cactusPos = cactusNivel2;
        monedasPos = monedasNivel2;
        cantidadCactus = 3;
        cantidadMonedas = 3;
    } else if (nivelActual == 2) {
        cactusPos = cactusNivel3;
        monedasPos = monedasNivel3;
        cantidadCactus = 4;
        cantidadMonedas = 4;
    }

    //  Verificar colisión con cactus
    for (int i = 0; i < cantidadCactus; i++) {
        if (x + 32 > cactusPos[i].x && x < cactusPos[i].x + 32 &&
            y + 32 > cactusPos[i].y && y < cactusPos[i].y + 32) {
            
            sonidoCactus(); //  Sonido al chocar con cactus
            vidas--;
            x = 0;
            y = pisoNiveles[nivelActual] - 32;
            //reinicia la posicion del dino 
            enSalto = false;
            // y pone que no este saltando 

         
    }

    //  Verificar colisión con monedas 
    for (int i = 0; i < cantidadMonedas; i++) {
        if (x + 32 > monedasPos[i].x && x < monedasPos[i].x + 32 &&
            y + 32 > monedasPos[i].y && y < monedasPos[i].y + 32) {

            //estas dos lineas detectan la colision ya que x y y son las coordenadas
            //del jugador , y verifica si esta entre ese espacio la posicion de lcada moneda
            
            sonidoMoneda(); //  Sonido al recoger moneda
            score += 10; // acumulador de puntos
            //borra la moneda de la pantalla
            screen.fillRect(monedasPos[i].x, monedasPos[i].y, 32, 32, ILI9341_BLACK);
            monedasPos[i].x = -100; //mueve la moneda fuera de la pantalla para que no se vuelva a detectar 
        }
    }

    //---victorias--
    if (score >= 80) { // Aquí se puede cambiar el numeor de putos que necsita el dino para ganar
    mostrarVictoria(); // Activar pantalla de victoria
    while (true); // Detener ejecución para que "YOU WIN" se muestre correctamente
    }//por lo que you win queda estatico muchooo tiempo
    //----------
    //gameover---
    if (vidas <= 0) {
    mostrarGameOverDino(); // Mostrar pantalla de Game Over
    while (true); // Bloquear ejecución para mostrar "GAME OVER"
    }
}
}
void dibujarObstaculos() {
    for (int i = 0; i < 2; i++) {
        screen.drawRGBBitmap(cactusNivel1[i].x, cactusNivel1[i].y, spriteCactus, 32, 32);
    }
    for (int i = 0; i < 3; i++) {
        screen.drawRGBBitmap(cactusNivel2[i].x, cactusNivel2[i].y, spriteCactus, 32, 32);
    }
    for (int i = 0; i < 4; i++) {
        screen.drawRGBBitmap(cactusNivel3[i].x, cactusNivel3[i].y, spriteCactus, 32, 32);
    }

    for (int i = 0; i < 2; i++) {
        screen.drawRGBBitmap(monedasNivel1[i].x, monedasNivel1[i].y, spriteMoneda, 32, 32);
    }
    for (int i = 0; i < 3; i++) {
        screen.drawRGBBitmap(monedasNivel2[i].x, monedasNivel2[i].y, spriteMoneda, 32, 32);
    }
    for (int i = 0; i < 4; i++) {
        screen.drawRGBBitmap(monedasNivel3[i].x, monedasNivel3[i].y, spriteMoneda, 32, 32);
    }
}

void verificarNivel() {
    if (x >= XMAX - 32) { // XMAX es el ancho de la pantalla , ese es el final del nivel
                            // y x la poscicion del dino 
        if (nivelActual < 2) {
            //  Borra la imagen anterior con negro antes de cambiar de nivel
            screen.fillRect(x, y, 32, 32, ILI9341_BLACK);

            nivelActual++;
            x = 0;
            y = pisoNiveles[nivelActual] - 32;
            enSalto = false;
        }
    }
}
// actualiza la posicion del dino
void setPlayerPosition(int x1, int y1) {// son las nuevas posciciones internamente
    x = x1;
    y = y1;
}


void animatePlayer(int frame) {
    //  Borra la posición anterior con negro antes de dibujar la nueva
    screen.fillRect(lastX, lastY, 32, 32, ILI9341_BLACK);

    //  Dibujar el jugador en la nueva posición
    screen.drawRGBBitmap(x, y, Player[frame], 32, 32);

    //  Actualizar la nueva posición para dibujarla en la pantalla
    lastX = x;
    lastY = y;
}

void moverPlayerDerecha() {
    moverPlayer(RIGHT);
}


void saltar() {
    //SALTA SI NO ESTA EN MEDIO DE OTRO SALTO
    if (!enSalto) { // Evita múltiples saltos consecutivos
        enSalto = true;

        // Borra la imagen anterior del dino con negro antes de moverlo
        screen.fillRect(x, y, 32, 32, ILI9341_BLACK);

        // Mueve el dino hacia arriba y adelante
        y -= alturaSalto; // esto es lo qeu simula la gravedad
        x += 50;

        lastY = y;
        lastX = x;
    }
}

void moverPlayer(uint8_t direccion) {// es un dato de 8 bits que represneta direcciones constatntes
    uint8_t delta = 10; //10 es la cantidad de pixeles que el jugador se movera en cada llamdo de la funcion
    switch (direccion) {
        case UP:
            y -= delta;
            break;
        case DOWN:
            y += delta;
            break;
        case LEFT:
            x -= delta;
            break;
        case RIGHT:
            x += delta;
            break;
    }
}

void mostrarVictoria() {
    screen.fillScreen(ILI9341_BLACK); //  Fondo negro

    //  Sonido de victoria
    for (int freq = 400; freq <= 1000; freq += 100) {
        tone(BUZZER_PIN, freq, 80);// activa el buzzer para emitir un sonido con la frecuencia
                        //durante 80 milisegundos
        delay(100);// espera 100 milisegundos antes de pasar al sgt tono
    }
    noTone(BUZZER_PIN); //apaga el buzzer para q deje de sonar

    // Texto más grande y correctamente centrado
    screen.setTextColor(ILI9341_GREEN);
    screen.setTextSize(5);

    // "YOU" centrado en la primera línea
    screen.setCursor((XMAX - (3 * 30)) / 2, YMAX / 2 - 50);//cada letra ocupa 30 px y hay 3 letras
    screen.print("YOU");

    // "WIN" centrado en la segunda línea
    screen.setCursor((XMAX - (3 * 30)) / 2, YMAX / 2);
    screen.print("WIN");

    // Dibujar el sprite de victoria bien alineado
    screen.drawRGBBitmap(XMAX / 2 - 32, YMAX / 2 + 60, spriteGameover, 64, 114);

    delay(3000); // Espera 3 segundos antes de pausar el juego
}
//-----------


void mostrarGameOverDino() {
    screen.fillScreen(ILI9341_BLACK); //  Fondo negro

    //  Efecto de sonido para Game Over
    for (int freq = 800; freq >= 200; freq -= 50) {
        tone(BUZZER_PIN, freq, 50);
        delay(60);
    }
    noTone(BUZZER_PIN);

    //  Texto más grande y centrado
    screen.setTextColor(ILI9341_RED);
    screen.setTextSize(5);
    int anchoTexto = 60;

    //  "GAME" en la primera línea
    screen.setCursor(XMAX / 2 - anchoTexto, YMAX / 2 - 50);
    screen.print("GAME");

    //  "OVER" en la segunda línea
    screen.setCursor(XMAX / 2 - anchoTexto, YMAX / 2);
    screen.print("OVER");

    //  Dibujar `spriteGameover` en lugar de `Player[1]`
    screen.drawRGBBitmap(XMAX / 2 - 32, YMAX / 2 + 60, spriteGameover, 64, 114);

    delay(3000); // Espera 3 segundos antes de pausar el juego
}

//sonidos

// Sonido de impacto con cactus
void sonidoCactus() {
    tone(BUZZER_PIN, 300, 200); //  Frecuencia baja, duración corta
}

// Sonido al recoger una moneda
void sonidoMoneda() {
    tone(BUZZER_PIN, 1000, 150); //  Frecuencia alta, duración breve
}

// Sonido de "Game Over"
void sonidoGameOver() {
    tone(BUZZER_PIN, 500, 800); //  Frecuencia media, duración larga
    delay(100);
    tone(BUZZER_PIN, 250, 800); //  Frecuencia más baja para efecto dramático
}
*/
