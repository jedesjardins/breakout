
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#define JED_LOG_IMPLEMENTATION
#include "log/logger.hpp"
#undef JED_LOG_IMPLEMENTATION

#define JED_GFX_IMPLEMENTATION
#include "gfx/render_device.hpp"
#undef JED_GFX_IMPLEMENTATION

#define JED_CMD_IMPLEMENTATION
#include "cmd/cmd.hpp"
#undef JED_CMD_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//
// INPUT
//

bool move_left{false};
bool move_right{false};
bool enter{false};

void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
    case GLFW_KEY_LEFT:
        move_left = action != GLFW_RELEASE;
        break;
    case GLFW_KEY_RIGHT:
        move_right = action != GLFW_RELEASE;
        break;
    case GLFW_KEY_ENTER:
        enter = action != GLFW_RELEASE;
        break;
    }
}

//
// OBJECT
//

class Object
{
public:
    gfx::BufferHandle vertex_buffer;
    gfx::BufferHandle index_buffer;
    size_t            index_count;

    glm::vec2 pos;
    glm::vec2 dim;

    glm::mat4 transform;

    Object(gfx::BufferHandle vertex_handle,
           gfx::BufferHandle index_handle,
           size_t            indices_count,
           glm::vec2 const & i_pos,
           glm::vec2 const & i_dim)
    : vertex_buffer{vertex_handle},
      index_buffer{index_handle},
      index_count{indices_count},
      pos{i_pos},
      dim{i_dim},
      transform{1.f}
    {
        transform = glm::scale(glm::translate(glm::mat4{1.f}, glm::vec3{pos.x, pos.y, 0.f}),
                               glm::vec3{dim.x, dim.y, 1.f});
    }

    std::pair<float, float> get_y_dim() const
    {
        return {pos.y, pos.y + dim.y};
    }

    std::pair<float, float> get_x_dim() const
    {
        return {pos.x, pos.x + dim.x};
    }

    void draw(gfx::Renderer &     render_device,
              gfx::PipelineHandle pipeline_handle,
              gfx::UniformHandle  view_handle)
    {
        render_device.draw(pipeline_handle,
                           vertex_buffer,
                           0,
                           index_buffer,
                           0,
                           index_count,
                           sizeof(glm::mat4),
                           glm::value_ptr(transform),
                           1,
                           &view_handle);
    }

    void draw(gfx::Renderer &     render_device,
              gfx::PipelineHandle pipeline_handle,
              gfx::UniformHandle  view_handle,
              glm::mat4 &         n_transform)
    {
        render_device.draw(pipeline_handle,
                           vertex_buffer,
                           0,
                           index_buffer,
                           0,
                           index_count,
                           sizeof(glm::mat4),
                           glm::value_ptr(n_transform),
                           1,
                           &view_handle);
    }
};

//
// BALL
//

struct Ball: public Object
{
public:
    glm::vec2 vel;

    Ball(gfx::BufferHandle vertex_handle,
         gfx::BufferHandle index_handle,
         uint32_t          indices_count,
         glm::vec2 const & i_pos,
         glm::vec2 const & i_dim,
         glm::vec2 const & i_vel)
    : Object{vertex_handle, index_handle, indices_count, i_pos, i_dim}, vel{i_vel}
    {}
};

//
// Collision
//

float overlap(std::pair<float, float> const & dim_1, std::pair<float, float> const & dim_2)
{
    if (dim_1.first < dim_2.second && dim_1.first > dim_2.first)
    {
        return dim_2.second - dim_1.first;
    }

    if (dim_1.second < dim_2.second && dim_1.second > dim_2.first)
    {
        return dim_2.first - dim_1.second;
    }

    return 0.f;
}

std::pair<float, float> SAT(Object const & paddle, Ball const & ball)
{
    auto paddle_x_axis = paddle.get_x_dim();
    auto paddle_y_axis = paddle.get_y_dim();

    auto ball_x_axis = ball.get_x_dim();
    auto ball_y_axis = ball.get_y_dim();

    auto overlap_x = overlap(ball_x_axis, paddle_x_axis);
    auto overlap_y = overlap(ball_y_axis, paddle_y_axis);

    return {overlap_x, overlap_y};
}

//
//	STATES
//

enum class State
{
    WAIT,
    PLAY,
    LOSE
};

State wait_state(Ball & ball, Object & paddle)
{
    if (enter)
    {
        return State::PLAY;
    }

    return State::WAIT;
}

State play_state(Ball & ball, Object & paddle, std::unordered_map<size_t, Object> & blocks)
{
    if (move_left)
    {
        paddle.pos.x -= .5f;

        if (paddle.pos.x < 0.f)
        {
            paddle.pos.x = 0.f;
        }
    }

    if (move_right)
    {
        paddle.pos.x += .5f;

        if (paddle.pos.x + paddle.dim.x > 60.f)
        {
            paddle.pos.x = 60.f - paddle.dim.x;
        }
    }

    ball.pos += ball.vel;

    if (ball.pos.x < 0)
    {
        ball.pos.x = 0;
        ball.vel.x *= -1;
    }

    if (ball.pos.x > 59)
    {
        ball.pos.x = 59;
        ball.vel.x *= -1;
    }

    if (ball.pos.y < 0)
    {
        ball.pos.y = 0;
        ball.vel.y *= -1;
    }

    if (ball.pos.y > 39)
    {
        return State::WAIT;
    }

    size_t block_to_destroy{};

    for (auto block_iter: blocks)
    {
    	auto & block = block_iter.second;

        auto overlap_pair = SAT(block, ball);
        if (overlap_pair.first != 0.f && overlap_pair.second != 0.f)
        {
            if (std::abs(overlap_pair.first) < std::abs(overlap_pair.second))
            {
                ball.pos.x += overlap_pair.first;
                ball.vel.x *= -1;
            }
            else
            {
                ball.pos.y += overlap_pair.second;
                ball.vel.y *= -1;
            }
            block_to_destroy = block_iter.first;
        }
    }

    blocks.erase(block_to_destroy);

    auto overlap_pair = SAT(paddle, ball);
    if (overlap_pair.first != 0.f && overlap_pair.second != 0.f)
    {
        if (std::abs(overlap_pair.first) < std::abs(overlap_pair.second))
        {
            ball.pos.x += overlap_pair.first;
            ball.vel.x *= 1;
        }
        else
        {
            LOG_INFO("Overlap was smaller on y axis");
            ball.pos.y += overlap_pair.second;

            float magnitude = std::sqrt((ball.vel.x*ball.vel.x) + (ball.vel.y*ball.vel.y));

            magnitude += 0.02f;

            LOG_INFO("Magnitude {}", magnitude);

            float diff = (ball.pos.x + (ball.dim.x/2)) - (paddle.pos.x + (paddle.dim.x/2));

            LOG_INFO("Diff {}", diff);

            float angle = (diff - 4) * -22.5;

            LOG_INFO("Angle {}", angle);

            ball.vel.x = std::cos((angle * 3.14)/180);
            ball.vel.y = -1 * std::sin((angle * 3.14)/180);

			ball.vel.x *= magnitude;
            ball.vel.y *= magnitude;

            LOG_INFO("Velocity {} {}\n", ball.vel.x, ball.vel.y);
        }
    }

    ball.transform = glm::scale(
        glm::translate(glm::mat4{1.f}, glm::vec3{ball.pos.x, ball.pos.y, 0.f}),
        glm::vec3{ball.dim.x, ball.dim.y, 1.f});

    paddle.transform = glm::scale(
        glm::translate(glm::mat4{1.f}, glm::vec3{paddle.pos.x, paddle.pos.y, 0.f}),
        glm::vec3{paddle.dim.x, paddle.dim.y, 1.f});

    return State::PLAY;
}

int main()
{
    get_console_sink()->set_level(spdlog::level::info);
    get_file_sink()->set_level(spdlog::level::debug);
    get_logger()->set_level(spdlog::level::debug);

    LOG_INFO("Starting Breakout");

    if (glfwInit() == GLFW_FALSE)
    {
        LOG_ERROR("GLFW didn't initialize correctly");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    auto window = glfwCreateWindow(600, 400, "Vulkan", nullptr, nullptr);

    auto render_device = gfx::Renderer{window};

    auto render_config = gfx::RenderConfig{.config_filename = "../breakout_config.json"};

    render_config.init();

    if (!render_device.init(render_config))
    {
        LOG_ERROR("Couldn't initialize the Renderer");
        return 0;
    }

    glfwSetKeyCallback(window, key_callback);

    //
    // View Uniform
    //

    auto uniform_layout_handle
        = render_device.get_uniform_layout_handle("ul_camera_matrix").value();

    auto view_matrix = glm::ortho(0.0f, 60.f, 0.0f, 40.f);

    auto view_uniform = render_device
                            .new_uniform(uniform_layout_handle,
                                         sizeof(glm::mat4),
                                         glm::value_ptr(view_matrix))
                            .value();

    //
    // Object buffers
    //

    std::vector<glm::vec3> vertices{
        {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {1.f, 1.f, 0.f}, {0.f, 1.f, 0.f}};

    std::vector<uint32_t> indices{0, 1, 2, 0, 2, 3};

    auto vertex_buffer = render_device
                             .create_buffer(vertices.size() * sizeof(glm::vec3),
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT
                                                | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                             .value();

    render_device.update_buffer(
        vertex_buffer, vertices.size() * sizeof(glm::vec3), vertices.data());

    auto index_buffer = render_device
                            .create_buffer(
                                indices.size() * sizeof(uint32_t),
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
                            .value();

    render_device.update_buffer(index_buffer, indices.size() * sizeof(uint32_t), indices.data());

    //
    // Pipeline Handle
    //

    auto pipeline_handle = render_device.get_pipeline_handle("white_box_shader").value();

    //
    // Objects
    //

    Object paddle = Object(vertex_buffer, index_buffer, 6, glm::vec2{27, 38}, glm::vec2{6, 1});
    Ball   ball   = Ball(
        vertex_buffer, index_buffer, 6, glm::vec2{29.5, 37}, glm::vec2{1, 1}, glm::vec2{.2, -.2});
    std::unordered_map<size_t, Object> blocks;

    size_t uuid{1};
    for (size_t i = 0; i < 12; ++i)
    {
        for (size_t j = 0; j < 5; ++j)
        {
            uuid++;
            blocks.emplace(uuid,
                           Object{vertex_buffer,
                                  index_buffer,
                                  6,
                                  glm::vec2{.5 + (i * 5), 2 + 2 * j},
                                  glm::vec2{4, 1}});
        }
    }

    //
    // Draw Loop
    //

    State current_state = State::WAIT;

    bool draw_success{true};
    while (!glfwWindowShouldClose(window) && draw_success)
    {
        glfwPollEvents();

        if (current_state == State::WAIT)
        {
            current_state = wait_state(ball, paddle);
        }
        else if (current_state == State::PLAY)
        {
            current_state = play_state(ball, paddle, blocks);
        }

        paddle.draw(render_device, pipeline_handle, view_uniform);
        ball.draw(render_device, pipeline_handle, view_uniform);

        for (auto iter: blocks)
        {
            iter.second.draw(render_device, pipeline_handle, view_uniform);
        }

        draw_success = render_device.submit_frame();
    }

    LOG_INFO("Stopping Breakout\n");

    glfwDestroyWindow(window);
    glfwTerminate();
}