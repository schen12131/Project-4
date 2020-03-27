#define GL_SILENCE_DEPRECATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define FIXED_TIMESTEP 0.0166666f

#include "Entity.h"
#define PLATFORM_COUNT 16
#define ENEMY_COUNT 3

struct GameState {
    Entity *player;
    Entity *platforms;
    Entity *enemies;
    Entity *gameOver;
    Entity *youWin;
};

GameState state;

SDL_Window* displayWindow;
bool gameIsRunning = true;

ShaderProgram program;
glm::mat4 viewMatrix, modelMatrix, projectionMatrix;

GLuint LoadTexture(const char* filePath) {
    int w, h, n;
    unsigned char* image = stbi_load(filePath, &w, &h, &n, STBI_rgb_alpha);
    
    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return textureID;
}


void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Textured!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    glViewport(0, 0, 640, 480);
    
    program.Load("shaders/vertex_textured.glsl", "shaders/fragment_textured.glsl");
    
    viewMatrix = glm::mat4(1.0f);
    modelMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    program.SetProjectionMatrix(projectionMatrix);
    program.SetViewMatrix(viewMatrix);
    
    glUseProgram(program.programID);
    
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_BLEND);
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    
    // Initialize Game Objects
    state.gameOver = new Entity[9];
    state.youWin = new Entity[7];
    
    float counter = -2.0f;
    
    GLuint fontTextureID = LoadTexture("font1.png");
    
    for (int i = 0; i < 9; i++){
        state.gameOver[i].textureID = fontTextureID;
        state.gameOver[i].position = glm::vec3(counter, 0, 0);
        state.gameOver[i].animCols = 16;
        state.gameOver[i].animRows = 16;
        state.gameOver[i].height = 0.3f;
        state.gameOver[i].width = 0.3f;
        state.gameOver[i].fail = new int[9] {71, 65, 77, 69, 9, 79, 86, 69, 82};
        state.gameOver[i].index = i;
        state.gameOver[i].isActive = false;
        
        if (i < 7){
            state.youWin[i].textureID = fontTextureID;
            state.youWin[i].position = glm::vec3(counter, 0, 0);
            state.youWin[i].animCols = 16;
            state.youWin[i].animRows = 16;
            state.youWin[i].height = 0.3f;
            state.youWin[i].width = 0.3f;
            state.youWin[i].success = new int[7] {89, 79, 85, 9, 87, 73, 78};
            state.youWin[i].index = i;
            state.youWin[i].isActive = false;
            
        }
        
        counter += 0.5f;
        
    }
    
    // Initialize Player
    state.player = new Entity();
    state.player->entityType = PLAYER;
    state.player->position = glm::vec3(-4, -1, 0);
    state.player->movement = glm::vec3(0);
    state.player->acceleration = glm::vec3(0, -9.81f, 0);
    state.player->speed = 1.0f;
    state.player->textureID = LoadTexture("george_0.png");
    
    state.player->animRight = new int[4] {3, 7, 11, 15};
    state.player->animLeft = new int[4] {1, 5, 9, 13};
    state.player->animUp = new int[4] {2, 6, 10, 14};
    state.player->animDown = new int[4] {0, 4, 8, 12};
    
    state.player->animIndices = state.player->animRight;
    state.player->animFrames = 4;
    state.player->animIndex = 0;
    state.player->animTime = 0;
    state.player->animCols = 4;
    state.player->animRows = 4;
    state.player->height = 0.8f;
    state.player->width = 0.8f;
    
    state.player->jumpPower = 7.0f;
    
    
    state.platforms = new Entity[PLATFORM_COUNT];
    GLuint platformTextureID = LoadTexture("platformPack_tile001.png");
    
    for (int i = 0; i < 11; i++){
        state.platforms[i].entityType = PLATFORM;
        state.platforms[i].textureID = platformTextureID;
        state.platforms[i].position = glm::vec3(-5 + i, -3.25, 0);
    }
    
    for (int j = 0; j < 6; j++){
        state.platforms[11 + j].entityType = PLATFORM;
        state.platforms[11 + j].textureID = platformTextureID;
        state.platforms[11 + j].position = glm::vec3(-5 + j, 0.5, 0);
    }
    
    
    for (int i = 0; i < PLATFORM_COUNT; i++){
        state.platforms[i].Update(0, NULL, NULL, 0);
    }
    
    state.enemies = new Entity[ENEMY_COUNT];
    GLuint enemyTextureID = LoadTexture("enemy2.png");
    
    state.enemies[0].entityType = ENEMY;
    state.enemies[0].aiType = WALKER;
    state.enemies[0].aiState = WALKING;
    state.enemies[0].textureID = enemyTextureID;
    state.enemies[0].position = glm::vec3(4, -2.25, 0);
    state.enemies[0].speed = 1;
    state.enemies[0].movement = glm::vec3(-1, 0, 0);
    
    state.enemies[1].entityType = ENEMY;
    state.enemies[1].aiType = WAITANDGO;
    state.enemies[1].aiState = IDLE;
    state.enemies[1].textureID = enemyTextureID;
    state.enemies[1].position = glm::vec3(4, -2.25, 0);
    state.enemies[1].speed = 1;
    
    state.enemies[2].entityType = ENEMY;
    state.enemies[2].aiType = JUMPER;
    state.enemies[2].textureID = enemyTextureID;
    state.enemies[2].position = glm::vec3(-4, 1.50, 0);
    state.enemies[2].speed = 1;
    state.enemies[2].movement = glm::vec3(0, 1, 0);
    state.enemies[2].speed = 1;
    state.enemies[2].acceleration = glm::vec3(0, -9.81f, 0);
    state.enemies[2].jumpPower = 4.0f;
    
    for (int j = 0; j < 9; j++){
        state.gameOver[j].Update(0, NULL, NULL, 0);
    }
    
    for (int k = 0; k < 7; k++){
        state.youWin[k].Update(0, NULL, NULL, 0);
    }
    
}

void ProcessInput() {
    
    state.player->movement = glm::vec3(0);
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                gameIsRunning = false;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_LEFT:
                        // Move the player left
                        break;
                        
                    case SDLK_RIGHT:
                        // Move the player right
                        break;
                        
                    case SDLK_SPACE:
                        // Some sort of action
                        //if (state.player->collidedBottom){
                            state.player->jump = true;
                        //}
                        break;
                }
                break; // SDL_KEYDOWN
        }
    }
    
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    
    if (keys[SDL_SCANCODE_LEFT]) {
        state.player->movement.x = -1.0f;
        state.player->animIndices = state.player->animLeft;
    }
    else if (keys[SDL_SCANCODE_RIGHT]) {
        state.player->movement.x = 1.0f;
        state.player->animIndices = state.player->animRight;
    }
    
    
    if (glm::length(state.player->movement) > 1.0f) {
        state.player->movement = glm::normalize(state.player->movement);
    }
    
}

float lastTicks = 0;
float accumulator = 0.0f;

void Update() {
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - lastTicks;
    lastTicks = ticks;
    deltaTime += accumulator;
    bool gameWon = false;
    if (deltaTime < FIXED_TIMESTEP) {
        accumulator = deltaTime;
        return; }
    //std::cout << "reset counter"<< std:: endl;
    while (deltaTime >= FIXED_TIMESTEP) {
        // Update. Notice it's FIXED_TIMESTEP. Not deltaTime
        state.player->Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT);
        state.player->Update(FIXED_TIMESTEP, state.player, state.enemies, ENEMY_COUNT);
        for (int i = 0; i < ENEMY_COUNT; i ++){
            state.enemies[i].Update(FIXED_TIMESTEP, state.player, state.platforms, PLATFORM_COUNT);
            if (state.player->collidedRight || state.player->collidedLeft || state.player->collidedTop){
                state.player->isActive = false;
            }
        }
        if (state.player->isActive == false){
            for (int i = 0; i < 9; i ++){
                state.gameOver[i].isActive = true;
                state.gameOver[i].Update(0, NULL, NULL, 0);
            }
        }
        
        if (state.enemies[0].isActive == false && state.enemies[1].isActive == false && state.enemies[2].isActive == false){
            gameWon = true;
        }
        
        if (gameWon == true){
            for (int i = 0; i < 7; i++){
                state.youWin[i].isActive = true;
                state.youWin[i].Update(0, NULL, NULL, 0);
            }
        }
        
        
        deltaTime -= FIXED_TIMESTEP;
    }
    accumulator = deltaTime;
}

void Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    for (int i = 0; i < PLATFORM_COUNT; i++){
        state.platforms[i].Render(&program);
    }
    
    for (int i = 0; i < ENEMY_COUNT; i++){
        state.enemies[i].Render(&program);
    }
    
    for(int j = 0; j < 9; j++){
        state.gameOver[j].Render(&program);
    }
    
    for(int k = 0; k < 7; k++){
        state.youWin[k].Render(&program);
    }
    
    state.player->Render(&program);
    
    SDL_GL_SwapWindow(displayWindow);
}


void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }
    
    Shutdown();
    return 0;
}
