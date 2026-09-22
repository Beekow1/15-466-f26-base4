#include "PlayMode.hpp"

#include "ColorTextureProgram.hpp"
#include "load_save_png.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/color_space.hpp>

#include <random>

#define CANVAS_WIDTH 320
#define CANVAS_HEIGHT 180

PlayMode::Image PlayMode::load_image(std::string const &filename)
{
	Image image;
	std::vector<glm::u8vec4> pixels;
	load_png(data_path(filename), &image.size, &pixels, UpperLeftOrigin);

	glGenTextures(1, &image.texture);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image.texture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, static_cast<GLsizei>(image.size.x), static_cast<GLsizei>(image.size.y), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_2D, 0);
	return image;
}

void PlayMode::generate_story()
{
	auto first = std::make_shared<StoryNode>();
	auto ask_info = std::make_shared<StoryNode>();
	auto refuse_first = std::make_shared<StoryNode>();
	auto returned = std::make_shared<StoryNode>();
	auto ask_real = std::make_shared<StoryNode>();

	auto sneak = std::make_shared<StoryNode>();
	auto accepted_fake = std::make_shared<StoryNode>();
	auto accepted_real = std::make_shared<StoryNode>();
	auto refused = std::make_shared<StoryNode>();

	glm::vec2 desk(135.0f, 55.0f);
	glm::vec2 outsideL(-50.0f, 55.0f);
	glm::vec2 outsideR(320.0f, 55.0f);

	first->text =
		"THE VISITOR HANDS YOU A VISA\n"
		"PLEASE LET ME THROUGH\n"
		"MY FAMILY IS WAITING";
	first->character_position = desk;
	first->visa = 1;
	first->choiceLText = "ASK FOR AGE AND HEIGHT";
	first->choiceL = ask_info;
	first->choiceRText = "REFUSE";
	first->choiceR = refuse_first;

	ask_info->text =
		"YOU ASK FOR AGE AND HEIGHT\n"
		"THE VISITOR SAYS\n"
		"FORTY AND SIX FOOT ONE";
	ask_info->character_position = desk;
	ask_info->lerp_time = 0.0f;
	ask_info->visa = 1;
	ask_info->choiceLText = "ACCEPT";
	ask_info->choiceL = accepted_fake;
	ask_info->choiceRText = "REFUSE";
	ask_info->choiceR = refuse_first;

	refuse_first->text =
		"I KNEW THAT OFFER WAS TOO GOOD TO BE TRUE\n"
		"THE VISITOR WALKS AWAY";
	refuse_first->character_position = outsideL;
	refuse_first->lerp_time = 0.8f;
	refuse_first->visa = 0;
	refuse_first->choiceLText = "WAIT";
	refuse_first->choiceL = returned;
	refuse_first->choiceRText = "TAKE A BREAK";
	refuse_first->choiceR = sneak;

	returned->text =
		"THE SAME VISITOR RETURNS\n"
		"SORRY THAT WAS MY SISTERS VISA\n"
		"HERE IS MY REAL VISA";
	returned->character_position = desk;
	returned->lerp_time = 0.8f;
	returned->visa = 2;
	returned->choiceLText = "ASK FOR AGE AND HEIGHT";
	returned->choiceL = ask_real;
	returned->choiceRText = "REFUSE";
	returned->choiceR = refused;

	ask_real->text =
		"FORTY AND SIX FOOT ONE\n"
		"THE DETAILS MATCH THIS VISA\n"
		"WILL YOU LET ME THROUGH";
	ask_real->character_position = desk;
	ask_real->lerp_time = 0.0f;
	ask_real->visa = 2;
	ask_real->choiceLText = "ACCEPT";
	ask_real->choiceL = accepted_real;
	ask_real->choiceRText = "REFUSE";
	ask_real->choiceR = refused;

	accepted_fake->text =
		"THE VISITOR HURRIES THROUGH\n"
		"YOU CHECK THE DETAILS AGAIN\n"
		"YOU ACCEPTED A FAKE VISA";
	accepted_fake->character_position = outsideR;
	accepted_fake->lerp_time = 0.8f;
	accepted_fake->visa = 0;

	accepted_real->text =
		"YOU APPROVE THE VISA\n"
		"THE VISITOR WAVES TO SOMEONE\n"
		"A FAMILY IS TOGETHER AGAIN";
	accepted_real->character_position = outsideR;
	accepted_real->lerp_time = 0.8f;
	accepted_real->visa = 0;

	refused->text =
		"YOU REFUSE ENTRY\n"
		"THE VISITOR LOWERS THEIR HEAD\n"
		"AND WALKS AWAY";
	refused->character_position = outsideL;
	refused->lerp_time = 0.8f;
	refused->visa = 0;

	sneak->text =
		"YOU RETURN FROM YOUR BREAK\n"
		"FOOTPRINTS LEAD PAST THE DESK\n"
		"SOMEONE SLIPPED THROUGH";
	sneak->character_position = outsideL;
	sneak->lerp_time = 0.0f;
	sneak->visa = 0;

	story.root = first;
	story.current = first;
}

PlayMode::PlayMode() : text(data_path("pixel_font.bdf"))
{
	SDL_SetWindowMinimumSize(Mode::window, CANVAS_WIDTH, CANVAS_HEIGHT);

	generate_story();

	character_position = story.current->character_position;
	last_position = character_position;
	char_timer = 0.0f;

	backdrop = load_image("Backdrop.png");
	character = load_image("Character.png");
	foreground = load_image("Foreground.png");

	fake_visa = load_image("Visa 2.png");
	real_visa = load_image("Visa 1.png");

	MeshBuffer::Vertex const vertices[] = {
		{{0, 0, 0}, glm::vec3(0.0f, 0.0f, 1.0f), glm::u8vec4(255, 255, 255, 255), {0, 0}},
		{{1, 0, 0}, glm::vec3(0.0f, 0.0f, 1.0f), glm::u8vec4(255, 255, 255, 255), {1, 0}},
		{{1, 1, 0}, glm::vec3(0.0f, 0.0f, 1.0f), glm::u8vec4(255, 255, 255, 255), {1, 1}},
		{{0, 0, 0}, glm::vec3(0.0f, 0.0f, 1.0f), glm::u8vec4(255, 255, 255, 255), {0, 0}},
		{{1, 1, 0}, glm::vec3(0.0f, 0.0f, 1.0f), glm::u8vec4(255, 255, 255, 255), {1, 1}},
		{{0, 1, 0}, glm::vec3(0.0f, 0.0f, 1.0f), glm::u8vec4(255, 255, 255, 255), {0, 1}}};

	glGenVertexArrays(1, &image_vao);
	glGenBuffers(1, &image_vbo);
	glBindVertexArray(image_vao);
	glBindBuffer(GL_ARRAY_BUFFER, image_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	auto const &program = *color_texture_program;

	glVertexAttribPointer(program.Position_vec4, 3, GL_FLOAT, GL_FALSE, sizeof(MeshBuffer::Vertex), reinterpret_cast<void *>(offsetof(MeshBuffer::Vertex, Position)));
	glEnableVertexAttribArray(program.Position_vec4);

	glVertexAttribPointer(program.Color_vec4, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(MeshBuffer::Vertex), reinterpret_cast<void *>(offsetof(MeshBuffer::Vertex, Color)));
	glEnableVertexAttribArray(program.Color_vec4);

	glVertexAttribPointer(program.TexCoord_vec2, 2, GL_FLOAT, GL_FALSE, sizeof(MeshBuffer::Vertex), reinterpret_cast<void *>(offsetof(MeshBuffer::Vertex, TexCoord)));
	glEnableVertexAttribArray(program.TexCoord_vec2);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void PlayMode::draw_image(Image const &image, glm::vec2 position)
{
	glm::mat4 transform(1.0f);
	transform[0][0] = 2.0f * float(image.size.x) / float(CANVAS_WIDTH);
	transform[1][1] = -2.0f * float(image.size.y) / float(CANVAS_HEIGHT);
	transform[3][0] = -1.0f + 2.0f * position.x / float(CANVAS_WIDTH);
	transform[3][1] = 1.0f - 2.0f * position.y / float(CANVAS_HEIGHT);

	auto const &program = *color_texture_program;
	glUseProgram(program.program);
	glUniformMatrix4fv(program.OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(transform));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, image.texture);
	glBindVertexArray(image_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	int width = static_cast<int>(drawable_size.x);
	int height = static_cast<int>(drawable_size.y);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_SCISSOR_TEST);
	glViewport(0, 0, width, height);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	draw_image(backdrop, glm::vec2(0.0f, 0.0f));
	draw_image(character, character_position);
	draw_image(foreground, glm::vec2(0.0f, 0.0f));

	if (!story.current)
	{
		throw "Unknown node";
	}

	StoryNode const &node = *story.current;

	if (char_timer <= 0.0f && node.visa != 0)
	{
		draw_image(node.visa == 1 ? fake_visa : real_visa, glm::vec2(0.0f));
	}

	std::string dialogue = node.text;

	if (char_timer <= 0.0f)
	{
		if (node.isEnding())
		{
			dialogue += "\n\nR  RESTART";
		}
		else
		{
			dialogue += "\n\nZ  " + node.choiceLText;
			dialogue += "\nX  " + node.choiceRText;
		}
	}

	text.draw_text(dialogue, glm::vec2(12.0f, 128.0f), 1, glm::vec4(glm::convertSRGBToLinear(glm::vec3(160.0f, 160.0f, 139.0f) / 255.0f), 1.0f), glm::uvec2(CANVAS_WIDTH, CANVAS_HEIGHT));
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{
	if (evt.type == SDL_EVENT_KEY_DOWN)
	{
		if (char_timer > 0.0f)
		{
			return true;
		}

		if (evt.key.key == SDLK_Z)
		{
			if (!evt.key.repeat && !story.current->isEnding())
			{
				last_position = character_position;

				story.chooseLeft();
				char_timer = story.current->lerp_time;
			}

			return true;
		}
		else if (evt.key.key == SDLK_X)
		{
			if (!evt.key.repeat && !story.current->isEnding())
			{
				last_position = character_position;

				story.chooseRight();
				char_timer = story.current->lerp_time;
			}

			return true;
		}
		else if (evt.key.key == SDLK_R)
		{
			if (!evt.key.repeat && story.current->isEnding())
			{
				last_position = character_position;

				story.restart();
				character_position = story.current->character_position;
				last_position = character_position;
				char_timer = story.current->lerp_time;
			}

			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	float duration = story.current->lerp_time;
	glm::vec2 target = story.current->character_position;

	if (duration <= 0.0f)
	{
		character_position = target;
		char_timer = 0.0f;
		return;
	}

	char_timer -= elapsed;
	if (char_timer > 0.0f)
	{
		float t = glm::clamp(1.0f - char_timer / duration, 0.0f, 1.0f);
		character_position = glm::mix(last_position, target, t);
	}
}
