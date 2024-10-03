#include <stdio.h>
#include <SDL2/SDL.h>
#include <math.h>  // For cos() and sin()

#define FALSE 0
#define TRUE 1

#define FPS 100
#define FRAME_TARGET_TIME (1000 / FPS)

#define PLAYER_SPEED 70.0f
#define PLAYER_ROTATION_SPEED 90.0f  // Degrees per second
#define SCREEN_WIDTH 1900
#define SCREEN_HEIGHT 1000
#define FOV 90  // Field of view in degrees
#define RAY_COUNT (SCREEN_WIDTH / 2)  // Number of rays to cast

#define MAP_WIDTH 10
#define MAP_HEIGHT 10

int game_is_running = FALSE;
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

int last_frame_time = 0;

float player_movement = 0;
float player_rotation = 0;  // Player's current rotation angle

struct ball {
    float x;
    float y;
    float width;
    float height;
    float rotation;  // Add rotation to track player's angle
} ball;

// Define the map data
int map[MAP_HEIGHT][MAP_WIDTH] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {1, 0, 1, 0, 0, 0, 0, 1, 1, 1},
    {1, 0, 1, 0, 1, 1, 0, 1, 0, 0},
    {1, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

int initialize_window(void) {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        fprintf(stderr, "Error initializing SDL. \n");
        return FALSE;
    }

    window = SDL_CreateWindow(
        "SDL Window",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    if (!window) {
        fprintf(stderr, "Error creating SDL Window. \n");
        return FALSE;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "Error creating SDL Renderer. \n");
        return FALSE;
    }

    return TRUE;
}

void process_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                game_is_running = FALSE;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    game_is_running = FALSE;
                }
                if (event.key.keysym.sym == SDLK_w) {
                    player_movement = 1;  // Move forward
                }
                if (event.key.keysym.sym == SDLK_s) {
                    player_movement = -1; // Move backward
                }
                if (event.key.keysym.sym == SDLK_a) {
                    player_rotation = -1;  // Rotate left
                }
                if (event.key.keysym.sym == SDLK_d) {
                    player_rotation = 1;  // Rotate right
                }
                break;
            case SDL_KEYUP:
                if (event.key.keysym.sym == SDLK_w || event.key.keysym.sym == SDLK_s) {
                    player_movement = 0;  // Stop movement when key is released
                }
                if (event.key.keysym.sym == SDLK_a || event.key.keysym.sym == SDLK_d) {
                    player_rotation = 0;  // Stop rotating when key is released
                }
                break;
        }
    }
}

void setup() {
    ball.x = 150;
    ball.y = 150;
    ball.width = 15;
    ball.height = 15;
    ball.rotation = 0;  // Initial rotation
}

void update() {
    float delta_time = (SDL_GetTicks() - last_frame_time) / 1000.0f;
    last_frame_time = SDL_GetTicks();

    ball.rotation += player_rotation * PLAYER_ROTATION_SPEED * delta_time;

    float angle_radians = ball.rotation * (M_PI / 180.0f);
    float direction_x = cos(angle_radians);
    float direction_y = sin(angle_radians);

    float new_x = ball.x + player_movement * PLAYER_SPEED * direction_x * delta_time;
    float new_y = ball.y + player_movement * PLAYER_SPEED * direction_y * delta_time;

    if (map[(int)(new_y / 30)][(int)(new_x / 30)] == 0) { // Ensure new position is not a wall
        ball.x = new_x;
        ball.y = new_y;
    }
}

// Optimized raycasting function
void cast_rays() {
    for (int i = 0; i < RAY_COUNT; i++) {
        float ray_angle = (ball.rotation - (FOV / 2.0f)) + ((float)i / RAY_COUNT) * FOV;
        float angle_radians = ray_angle * (M_PI / 180.0f);
        
        // Precalculate cosine and sine values
        float cos_angle = cos(angle_radians);
        float sin_angle = sin(angle_radians);

        float ray_x = ball.x;
        float ray_y = ball.y;

        float step_size = 1;  // Increase step size for faster raycasting
        float distance_to_wall = 0;

        // Use integer math for grid checks
        int map_x, map_y;
        while (distance_to_wall < SCREEN_WIDTH) {
            ray_x += cos_angle * step_size;
            ray_y += sin_angle * step_size;
            distance_to_wall += step_size;

            map_x = (int)(ray_x / 30);
            map_y = (int)(ray_y / 30);

            // Check if we've hit a wall
            if (map_y < 0 || map_y >= MAP_HEIGHT || map_x < 0 || map_x >= MAP_WIDTH) {
                break;  // Out of bounds
            }

            if (map[map_y][map_x] == 1) {
                break;  // Hit a wall
            }
        }

        // Draw the wall slice
        int line_height = (SCREEN_HEIGHT / distance_to_wall) * 20;
        int line_start = (SCREEN_HEIGHT / 2) - (line_height / 2);
        int wcolor = distance_to_wall * -2;
        int color = distance_to_wall * -2;

        if (wcolor < -255) {
            wcolor = 10;
        }

        if (color < 1) {
            color = 10;
        }
        if (color < -100) {
            color = 30;
        }

        // Draw ceiling
        SDL_SetRenderDrawColor(renderer, color, color, color, 255); // Light blue color for ceiling
        SDL_RenderDrawLine(renderer, i * 2, 0, i * 2, line_start);

        // Draw wall slice
        SDL_SetRenderDrawColor(renderer, wcolor, wcolor, wcolor, 255); // Set wall color
        SDL_RenderDrawLine(renderer, i * 2, line_start, i * 2, line_start + line_height);

        // Draw floor
        SDL_SetRenderDrawColor(renderer, color, color, color, 255); // Brown color for floor
        SDL_RenderDrawLine(renderer, i * 2, line_start + line_height, i * 2, SCREEN_HEIGHT);
    }
}

void render() {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Cast rays to render walls
    cast_rays();

    // Draw the rectangle (player)
    SDL_Rect ball_rect = {
        (int)ball.x,
        (int)ball.y,
        (int)ball.width,
        (int)ball.height
    };

    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &ball_rect);

    SDL_RenderPresent(renderer);
}

void destroy_window() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main() {
    game_is_running = initialize_window();

    setup();
    // Game loop
    while (game_is_running) {
        process_input();
        update();
        render();  // Draw
    }

    destroy_window();

    return 0;
}

DNS SPOOF

#include <pcap.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

// Fonction pour afficher les informations sur les paquets DNS
void print_dns_info(const u_char *packet, struct pcap_pkthdr header) {
    struct ip *ip_header = (struct ip*)(packet + 14); // Ignorer l'en-tête Ethernet (14 octets)
    struct udphdr *udp_header = (struct udphdr*)(packet + 14 + (ip_header->ip_hl * 4));
    
    printf("\n--- Paquet DNS capturé ---\n");
    printf("Taille du paquet : %d octets\n", header.len);
    printf("Source IP : %s\n", inet_ntoa(ip_header->ip_src));
    printf("Destination IP : %s\n", inet_ntoa(ip_header->ip_dst));
    printf("Source Port : %d\n", ntohs(udp_header->uh_sport));
    printf("Destination Port : %d\n", ntohs(udp_header->uh_dport));
    
    // Affichage des données DNS
    const u_char *dns_header = packet + 14 + (ip_header->ip_hl * 4) + sizeof(struct udphdr);
    printf("Données DNS : ");
    for (int i = 0; i < header.len - (14 + (ip_header->ip_hl * 4) + sizeof(struct udphdr)); i++) {
        printf("%02x ", dns_header[i]);
    }
    printf("\n");
}

// Fonction pour traiter les paquets capturés
void process_packet(unsigned char *args, const struct pcap_pkthdr *header, const unsigned char *packet) {
    // Analyser et imprimer les informations sur les paquets DNS
    print_dns_info(packet, *header);
}

int main() {
    char *dev = "enp0s3";  // Remplacer par le nom correct de l'interface
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return 1;
    }
    
    pcap_loop(handle, 0, process_packet, NULL);  // Capture continue
    
    pcap_close(handle);
    return 0;
}
