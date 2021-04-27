#pragma once
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"
class Camera {
public:


	Camera() {
		m_pos = { 0.f, -6.f, -10.f };
		m_direction = { 00.0f, 0.0f, -1.0 };
		m_up = { 0.0f,1.0f,.0f };
		update();
	}

	void rotate(float rad, glm::vec3& axis) {
		glm::mat4 rotationMat(1);
		currentDegree += rad*0.8;
		if (currentDegree >= 90.f) {

			rad = 89.0 - currentDegree;
			currentDegree = 89.f;
		}

		if (currentDegree <= -90.f) {

			rad = -89.0 - currentDegree;
			currentDegree = -89.f;
		}

		printf("degree  : %f \n", currentDegree);

		rotationMat = glm::rotate(rotationMat, glm::radians(rad), axis);
		m_direction = glm::vec3(rotationMat * glm::vec4(m_direction, 1.0f));
	}

	void translate(glm::vec3& dir)
	{
		m_pos += dir;
	}
	void update() {
		m_view = glm::lookAt(m_pos, m_pos + m_direction, m_up);
	}


	inline glm::mat4& getView() { return m_view; }

private:
	glm::vec3 m_pos;
	glm::vec3 m_direction;
	glm::mat4 m_view;
	glm::vec3 m_up;
	float currentDegree = 0.0;;
};
