#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>

#define MAX_BULLETS 1000

typedef struct {
  float x, y, dy;
  short life;
  char *name;
  int currentSprite, walking, facingLeft, shooting, visible;
  int alive;
  SDL_Texture *sheetTexture;
} Man;

typedef struct {
  float x, y, dx;
} Bullet;

SDL_Texture *bulletTexture;
SDL_Texture *backgroundTexture;
Bullet *bullets[MAX_BULLETS];
Man enemy;

int globalTime = 0;

void addBullet(float x, float y, float dx);
void removeBullet(int i);
int processEvents(SDL_Window *window, Man *man);
void doRender(SDL_Renderer *renderer, Man *man);
void updateLogic(Man *man);

void addBullet(float x, float y, float dx) {
  int found = -1;
  int i;
  for (i = 0; i < MAX_BULLETS; i++) {
    if (bullets[i] == NULL) {
      found = i;
      break;
    }
  }

  if (found >= 0) {
    i = found;
    bullets[i] = (Bullet *)malloc(sizeof(Bullet));
    bullets[i]->x = x;
    bullets[i]->y = y;
    bullets[i]->dx = dx;
  }
}

void removeBullet(int i) {
  if (bullets[i]) {
    free(bullets[i]);
    bullets[i] = NULL;
  }
}

int processEvents(SDL_Window *window, Man *man) {
  SDL_Event event;
  int done = 0;
  const Uint8 *state;

  while (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_WINDOWEVENT_CLOSE: {
      if (window) {
        SDL_DestroyWindow(window);
        window = NULL;
        done = 1;
      }
    } break;
    case SDL_KEYDOWN: {
      switch (event.key.keysym.sym) {
      case SDLK_ESCAPE:
        done = 1;
        break;
      default:
        break;
      }
    } break;
    case SDL_QUIT:
      /* quit out of the game */
      done = 1;
      break;
    default:
      break;
    }
  }

  state = SDL_GetKeyboardState(NULL);
  if (!man->shooting) {
    if (state[SDL_SCANCODE_LEFT]) {
      man->x -= 3;
      man->walking = 1;
      man->facingLeft = 1;

      if (globalTime % 6 == 0) {
        man->currentSprite++;
        man->currentSprite %= 4;
      }
    } else if (state[SDL_SCANCODE_RIGHT]) {
      man->x += 3;
      man->walking = 1;
      man->facingLeft = 0;

      if (globalTime % 6 == 0) {
        man->currentSprite++;
        man->currentSprite %= 4;
      }
    } else {
      man->walking = 0;
      man->currentSprite = 4;
    }
  }

  if (!man->walking) {
    if (state[SDL_SCANCODE_SPACE]) /* && !man->dy) */
    {
      if (globalTime % 6 == 0) {
        if (man->currentSprite == 4)
          man->currentSprite = 5;
        else
          man->currentSprite = 4;

        if (!man->facingLeft) {
          addBullet(man->x + 35, man->y + 20, 3);
        } else {
          addBullet(man->x + 5, man->y + 20, -3);
        }
      }

      man->shooting = 1;
    } else {
      man->currentSprite = 4;
      man->shooting = 0;
    }
  }

  if (state[SDL_SCANCODE_UP] && !man->dy) {
    man->dy = -8;
  }
  if (state[SDL_SCANCODE_DOWN]) {
    /* man->y += 10; */
  }

  return done;
}

void doRender(SDL_Renderer *renderer, Man *man) {
  int i;
  /* set the drawing color to blue */
  SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

  /* Clear the screen (to blue) */
  SDL_RenderClear(renderer);

  /* set the drawing color to white */
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

  /* SDL_RenderFillRect(renderer, &rect); */
  SDL_RenderCopy(renderer, backgroundTexture, NULL, NULL);

  /* warrior */
  if (man->visible) {
    SDL_Rect srcRect;
    SDL_Rect rect;

    srcRect.x = 40 * man->currentSprite;
    srcRect.y = 0;
    srcRect.w = 40;
    srcRect.h = 50;

    rect.x = (int)man->x;
    rect.y = (int)man->y;
    rect.w = 40;
    rect.h = 50;
    SDL_RenderCopyEx(renderer, man->sheetTexture, &srcRect, &rect, 0, NULL,
                     (SDL_RendererFlip)man->facingLeft);
  }

  /* enemy */
  if (enemy.visible) {
    SDL_Rect eSrcRect;
    SDL_Rect eRect;

    eSrcRect.x = 40 * 40 * enemy.currentSprite;
    eSrcRect.y = 0;
    eSrcRect.w = 40;
    eSrcRect.h = 50;

    eRect.x = (int)enemy.x;
    eRect.y = (int)enemy.y;
    eRect.w = 40;
    eRect.h = 50;

    SDL_RenderCopyEx(renderer, enemy.sheetTexture, &eSrcRect, &eRect, 0, NULL,
                     (SDL_RendererFlip)enemy.facingLeft);
  }

  for (i = 0; i < MAX_BULLETS; i++)
    if (bullets[i]) {
      SDL_Rect rect;

      rect.x = (int)bullets[i]->x;
      rect.y = (int)bullets[i]->y;
      rect.w = 8;
      rect.h = 8;

      SDL_RenderCopy(renderer, bulletTexture, NULL, &rect);
    }

  /* We are done drawing, "present" or show to the screen what we've drawn */
  SDL_RenderPresent(renderer);
}

void updateLogic(Man *man) {
  int i;
  man->y += man->dy;
  man->dy += 0.5f;
  if (man->y > 60) {
    man->y = 60;
    man->dy = 0;
  }

  for (i = 0; i < MAX_BULLETS; i++)
    if (bullets[i]) {
      bullets[i]->x += bullets[i]->dx;

      /* simple coll. detection */
      if (bullets[i]->x > enemy.x && bullets[i]->x < enemy.x + 40 &&
          bullets[i]->y > enemy.y && bullets[i]->y < enemy.y + 50) {
        enemy.alive = 0;
      }

      if (bullets[i]->x < -1000 || bullets[i]->x > 1000)
        removeBullet(i);
    }

  if (enemy.alive == 0 && globalTime % 6 == 0) {
    if (enemy.currentSprite < 6)
      enemy.currentSprite = 6;
    else if (enemy.currentSprite >= 6) {
      enemy.currentSprite++;
      if (enemy.currentSprite > 7) {
        enemy.visible = 0;
        enemy.currentSprite = 7;
      }
    }
  }

  globalTime++;
}

int main(void) {
  Man man;
  SDL_Window *window;     /* Declare a window */
  SDL_Renderer *renderer; /* Declare a renderer */
  SDL_Surface *sheet;
  SDL_Surface *bg;
  SDL_Surface *bullet;
  int done;
  int i;

  SDL_Init(SDL_INIT_VIDEO); /* Initialize SDL2 */

  man.x = 50;
  man.y = 0;
  man.currentSprite = 4;
  man.alive = 1;
  man.visible = 1;
  man.facingLeft = 0;

  enemy.x = 250;
  enemy.y = 60;
  enemy.currentSprite = 4;
  enemy.facingLeft = 1;
  enemy.alive = 1;
  enemy.visible = 1;

  /* Create an application window with the following settings: */
  window = SDL_CreateWindow("Game Window",           /* window title */
                            SDL_WINDOWPOS_UNDEFINED, /* initial x position */
                            SDL_WINDOWPOS_UNDEFINED, /* initial y position */
                            640,                     /* width, in pixels */
                            480,                     /* height, in pixels */
                            0                        /* flags */
  );
  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  SDL_RenderSetLogicalSize(renderer, 320, 240);

  sheet = IMG_Load("sheet.png");
  if (!sheet) {
    printf("Cannot find sheet\n");
    puts(IMG_GetError());
    return 1;
  }

  man.sheetTexture = SDL_CreateTextureFromSurface(renderer, sheet);
  SDL_FreeSurface(sheet);

  /* load enemy */
  sheet = IMG_Load("badman_sheet.png");
  if (!sheet) {
    printf("Cannot find enemy sheet\n");
    return 1;
  }

  enemy.sheetTexture = SDL_CreateTextureFromSurface(renderer, sheet);
  SDL_FreeSurface(sheet);

  /* load the bg */
  bg = IMG_Load("background.png");

  if (!sheet) {
    printf("Cannot find background\n");
    return 1;
  }

  backgroundTexture = SDL_CreateTextureFromSurface(renderer, bg);
  SDL_FreeSurface(bg);

  /* load the bullet */
  bullet = IMG_Load("bullet.png");

  if (!bullet) {
    printf("Cannot find bullet\n");
    return 1;
  }

  bulletTexture = SDL_CreateTextureFromSurface(renderer, bullet);
  SDL_FreeSurface(bullet);

  /* The window is open: enter program loop (see SDL_PollEvent) */
  done = 0;

  /* Event loop */
  while (!done) {
    /* Check for events */
    done = processEvents(window, &man);

    /* Update logic */
    updateLogic(&man);

    /* Render display */
    doRender(renderer, &man);

    /* don't burn up the CPU */
    SDL_Delay(10);
  }

  /* Close and destroy the window */
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyTexture(man.sheetTexture);
  SDL_DestroyTexture(backgroundTexture);
  SDL_DestroyTexture(bulletTexture);
  SDL_DestroyTexture(enemy.sheetTexture);

  for (i = 0; i < MAX_BULLETS; i++)
    removeBullet(i);

  /* Clean up */
  SDL_Quit();
  return 0;
}
