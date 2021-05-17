#pragma once
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
class Camera {
public:


	Camera() :m_pos({ 40.f, 50.f, -30.f }), m_translation(glm::vec3{ 0.0f }), m_up({ 0.0f,1.0f,.0f }) {

		update();
	}

	void rotate(const float yaw, const float pitch, const float roll) {
		m_yaw = yaw;
		m_pitch = pitch;
		m_roll = roll;
	}

	// moves forward and backward 
	void walk(const float amount)
	{
		m_translation += (m_look * amount);
	}

	void strafe(const float amount) {

		m_translation += (m_right * amount);
	}

	void lift(const float amount) {
		m_translation += (m_up * amount);
	}

	void update() {



		glm::mat4 r = getRotatioMatrix(m_yaw, m_pitch, m_roll);
		m_pos += m_translation;
		m_translation = glm::vec3(0);

		m_look = glm::vec3(r * glm::vec4(0, 0, 1, 0));
		m_up = glm::vec3(r * glm::vec4(0, 1, 0, 0));
		m_right = glm::cross(m_look, m_up);

		glm::vec3 target = m_pos + m_look;

		m_view = glm::lookAt(m_pos, target, m_up);
	}

	glm::mat4 getRotatioMatrix(const float yaw, const float pitch, const float roll) {
		glm::mat4 r = glm::mat4(1);
		r = glm::rotate(r, roll, glm::vec3(0, 0, 1));
		r = glm::rotate(r, yaw, glm::vec3(0, 1, 0));
		r = glm::rotate(r, pitch, glm::vec3(1, 0, 0));
		return r;
	}

	inline glm::mat4& getView() { return m_view; }

private:
	glm::vec3 m_pos;
	glm::vec3 m_up;
	glm::vec3 m_look;
	glm::vec3 m_right;
	glm::vec3 m_translation;

	float m_yaw, m_pitch, m_roll;

	glm::mat4 m_view;

	float currentDegree = 0.0;;
};
