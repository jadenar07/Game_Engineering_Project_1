/**
* Author: Jaden Ritchie
* Assignment: Simple 2D Scene
* Date due: 2023-09-20, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 700 * 2,
              WINDOW_HEIGHT = 500 * 2;

constexpr float BG_RED     = 0.9765625f,
                BG_GREEN   = 0.97265625f,
                BG_BLUE    = 0.9609375f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1,
                LEVEL_OF_DETAIL    = 0,
                TEXTURE_BORDER     = 0;

constexpr float G_GROWTH_FACTOR = 1.01f;
constexpr float G_SHRINK_FACTOR = 0.99f;
constexpr int   G_MAX_FRAME     = 40;

constexpr float G_ROT_ANGLE  = glm::radians(1.5f);
constexpr float G_TRAN_VALUE = 0.025f;

constexpr float BASE_SCALE = 2.0f,
                MAX_AMPLITUDE = 0.01f,
                PULSE_SPEED = 10.0f;

bool g_is_growing   = true;
int g_frame_counter = 0;

constexpr char SPIDERMAN_FILEPATH[]    = "spiderman.png",
               NARUTO_FILEPATH[] = "naruto.png";

constexpr glm::vec3 INIT_SCALE       = glm::vec3(2.0f, 2.98f, 0.0f),
//                    INIT_POS_SPIDERMAN    = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_NARUTO = glm::vec3(0.0f, 0.5f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_spiderman_matrix,
          g_naruto_matrix,
          g_projection_matrix;

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_spiderman = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_naruto = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_spiderman_texture_id,
       g_naruto_texture_id;


GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Hello, Textures!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_spiderman_matrix       = glm::mat4(1.0f);
    g_naruto_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    g_spiderman_texture_id   = load_texture(SPIDERMAN_FILEPATH);
    g_naruto_texture_id = load_texture(NARUTO_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}

constexpr float SPIDERMAN_ORBIT_RADIUS = 3.0f;
float SPIDERMAN_ORBIT_ANGLE = 0.0f;

void update()
{
    g_frame_counter++;
    glm::vec3 translation_vector;
    glm::vec3 scale_vector;
//    glm::vec3 rotation_triggers;
    
    
    if (g_frame_counter >= G_MAX_FRAME)
    {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }
        
    translation_vector = glm::vec3(G_TRAN_VALUE, G_TRAN_VALUE, 0.0f);
    glm::vec3 rotation_axis = glm::vec3(0.0f, 0.0f, 1.0f);
    scale_vector       = glm::vec3(
                                   g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
                                   g_is_growing ? G_GROWTH_FACTOR : G_SHRINK_FACTOR,
                                   1.0f
                                   );
    
    float scale_factor = BASE_SCALE + MAX_AMPLITUDE * glm::cos(g_frame_counter / PULSE_SPEED);
    glm::vec3 scale_factors = glm::vec3(scale_factor, scale_factor, 0.0f);

    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_rotation_spiderman.z += ROT_INCREMENT * delta_time;
    g_rotation_naruto.y += -1 * ROT_INCREMENT * delta_time;

    g_spiderman_matrix    = glm::mat4(1.0f);
    g_naruto_matrix = glm::mat4(1.0f);
    
    SPIDERMAN_ORBIT_ANGLE += ROT_INCREMENT*delta_time;
    float spiderman_x = SPIDERMAN_ORBIT_RADIUS * cos(SPIDERMAN_ORBIT_ANGLE);
    float spiderman_y = SPIDERMAN_ORBIT_RADIUS * sin(SPIDERMAN_ORBIT_ANGLE);

    
    g_spiderman_matrix = glm::translate(g_spiderman_matrix, INIT_POS_NARUTO);
    g_spiderman_matrix = glm::translate(g_spiderman_matrix, glm::vec3(spiderman_x, spiderman_y, 0.0f));
    g_spiderman_matrix = glm::rotate(g_spiderman_matrix, g_rotation_spiderman.z, rotation_axis);
    g_spiderman_matrix = glm::scale(g_spiderman_matrix, INIT_SCALE);
//    g_spiderman_matrix = glm::translate(g_spiderman_matrix, INIT_POS_NARUTO);


    g_naruto_matrix = glm::translate(g_naruto_matrix, INIT_POS_NARUTO);
    g_naruto_matrix = glm::rotate(g_naruto_matrix,
                                  g_rotation_naruto.y,
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    g_naruto_matrix = glm::scale(g_naruto_matrix, scale_factors);
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    draw_object(g_spiderman_matrix, g_spiderman_texture_id);
    draw_object(g_naruto_matrix, g_naruto_texture_id);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
} 
