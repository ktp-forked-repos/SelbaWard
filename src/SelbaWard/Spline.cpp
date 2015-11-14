//////////////////////////////////////////////////////////////////////////////
//
// Selba Ward (https://github.com/Hapaxia/SelbaWard)
// --
//
// Spline
//
// Copyright(c) 2014-2015 M.J.Silk
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions :
//
// 1. The origin of this software must not be misrepresented; you must not
// claim that you wrote the original software.If you use this software
// in a product, an acknowledgment in the product documentation would be
// appreciated but is not required.
//
// 2. Altered source versions must be plainly marked as such, and must not be
// misrepresented as being the original software.
//
// 3. This notice may not be removed or altered from any source distribution.
//
// M.J.Silk
// MJSilk2@gmail.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Spline.hpp"

#include <cmath>

namespace
{

inline sf::Vector2f linearInterpolation(const sf::Vector2f& start, const sf::Vector2f& end, const float& alpha)
{
	return (end * alpha + start * (1 - alpha));
}

inline sf::Vector2f bezierInterpolation(const sf::Vector2f& start, const sf::Vector2f& end, const sf::Vector2f& startHandle, const sf::Vector2f& endHandle, const float& alpha)
{
	const sf::Vector2f startControl{ start + startHandle };
	const sf::Vector2f endControl{ end + endHandle };
	const float alpha2{ 1.f - alpha };
	const float alphaSquared{ alpha * alpha };
	const float alpha2Squared{ alpha2 * alpha2 };

	sf::Vector2f p;

	p.x = start.x * alpha2Squared * alpha2 + startControl.x * 3 * alpha2Squared * alpha + endControl.x * 3 * alpha2 * alphaSquared + end.x * alpha * alphaSquared;
	p.y = start.y * alpha2Squared * alpha2 + startControl.y * 3 * alpha2Squared * alpha + endControl.y * 3 * alpha2 * alphaSquared + end.y * alpha * alphaSquared;

	return p;
}

inline float vectorLength(const sf::Vector2f& vector)
{
	return std::sqrt(vector.x * vector.x + vector.y * vector.y);
}

inline void copyAngle(const sf::Vector2f& source, sf::Vector2f& destination)
{
	destination = -(source / vectorLength(source)) * vectorLength(destination);
}

} // namespace

namespace selbaward
{

Spline::Spline()
	: m_exceptionPrefix("Spline: ")
	, m_throwExceptions(true)
	, m_vertices()
	, m_color(sf::Color::White)
	, m_sfmlVertices()
	, m_primitiveType(sf::PrimitiveType::LinesStrip)
	, m_interpolationSteps(0u)
	, m_handlesVertices()
	, m_showHandles(false)
	, m_useBezier(false)
	, m_lockHandleMirror(true)
	, m_lockHandleAngle(true)
{
}

void Spline::update()
{
	if (m_vertices.size() == 0)
	{
		m_sfmlVertices.clear();
		return;
	}

	const unsigned int pointsPerVertex{ m_interpolationSteps + 1u };
	m_sfmlVertices.resize(((m_vertices.size() - 1) * pointsPerVertex) + 1);

	m_handlesVertices.resize((m_vertices.size()) * 4);
	std::vector<sf::Vertex>::iterator itHandle = m_handlesVertices.begin();

	for (std::vector<Vertex>::iterator begin = m_vertices.begin(), end = m_vertices.end(), it = begin; it != end; ++it)
	{
		itHandle->color = sf::Color(255, 255, 128, 32);
		itHandle++->position = it->position;
		itHandle->color = sf::Color(0, 255, 0, 128);
		itHandle++->position = it->position + it->backHandle;
		itHandle->color = sf::Color(255, 255, 128, 32);
		itHandle++->position = it->position;
		itHandle->color = sf::Color(0, 255, 0, 128);
		itHandle++->position = it->position + it->frontHandle;

		std::vector<sf::Vertex>::iterator itSfml = m_sfmlVertices.begin() + (it - begin) * pointsPerVertex;
		if (it != end - 1)
		{
			for (unsigned int i{ 0u }; i < pointsPerVertex; ++i)
			{
				if (m_useBezier)
					itSfml->position = bezierInterpolation(it->position, (it + 1)->position, it->frontHandle, (it + 1)->backHandle, static_cast<float>(i) / pointsPerVertex);
				else
					itSfml->position = linearInterpolation(it->position, (it + 1)->position, static_cast<float>(i) / pointsPerVertex);
				itSfml->color = m_color;
				++itSfml;
			}
		}
		else
		{
			itSfml->position = it->position;
			itSfml->color = m_color;
		}
	}
}

void Spline::reserveVertices(const unsigned int numberOfVertices)
{
	if (numberOfVertices == 0)
		return;

	m_vertices.reserve(numberOfVertices);
	m_sfmlVertices.reserve((numberOfVertices - 1) * m_interpolationSteps + 1);
	m_handlesVertices.reserve(numberOfVertices * 4);
}

void Spline::addVertices(const std::vector<sf::Vector2f>& positions)
{
	for (auto& position : positions)
		addVertex(position);
}

void Spline::addVertices(const unsigned int index, const std::vector<sf::Vector2f>& positions)
{
	for (auto& position : positions)
		addVertex(index, position);
}

void Spline::addVertices(const unsigned int numberOfVertices, const sf::Vector2f position)
{
	for (unsigned int i{ 0 }; i < numberOfVertices; ++i)
		addVertex(position);
}

void Spline::addVertices(const unsigned int numberOfVertices, const unsigned int index, const sf::Vector2f position)
{
	for (unsigned int i{ 0 }; i < numberOfVertices; ++i)
		addVertex(index + i, position);
}

void Spline::addVertex(const sf::Vector2f position)
{
	m_vertices.emplace_back(Vertex(position));
}

void Spline::addVertex(const unsigned int index, const sf::Vector2f position)
{
	if (index < getVertexCount())
		m_vertices.insert(m_vertices.begin() + index, Vertex(position));
	else
		addVertex(position);
}

void Spline::removeVertex(const unsigned int index)
{
	if (!priv_testVertexIndex(index, "Cannot remove vertex."))
		return;

	m_vertices.erase(m_vertices.begin() + index);
}

void Spline::removeVertices(const unsigned int index, const unsigned int numberOfVertices)
{
	if (!priv_testVertexIndex(index, "Cannot remove vertices") || (numberOfVertices > 1 && !priv_testVertexIndex(index + numberOfVertices - 1, "Cannot remove vertices")))
		return;

	if (numberOfVertices == 0)
		m_vertices.erase(m_vertices.begin() + index, m_vertices.end());
	else
		m_vertices.erase(m_vertices.begin() + index, m_vertices.begin() + index + numberOfVertices);
}

void Spline::reverseVertices()
{
	std::reverse(m_vertices.begin(), m_vertices.end());

	// swap all handles
	sf::Vector2f tempHandle;
	for (auto& vertex : m_vertices)
	{
		tempHandle = vertex.frontHandle;
		vertex.frontHandle = vertex.backHandle;
		vertex.backHandle = tempHandle;
	}
}

void Spline::setPosition(const unsigned int index, const sf::Vector2f position)
{
	if (!priv_testVertexIndex(index, "Cannot set vertex position."))
		return;

	m_vertices[index].position = position;
}

void Spline::setPositions(const unsigned int index, unsigned int numberOfVertices, const sf::Vector2f position)
{
	if (!priv_testVertexIndex(index, "Cannot set vertices' positions") || (numberOfVertices > 1 && !priv_testVertexIndex(index + numberOfVertices - 1, "Cannot set vertices' positions")))
		return;

	if (numberOfVertices == 0)
		numberOfVertices = m_vertices.size() - index;

	for (unsigned int v{ 0u }; v < numberOfVertices; ++v)
		m_vertices[index + v].position = position;
}

void Spline::setPositions(const std::vector<sf::Vector2f>& positions, unsigned int index)
{
	const unsigned int numberOfVertices{ positions.size() };
	if (numberOfVertices < 1 || !priv_testVertexIndex(index, "Cannot set vertices' positions") || (numberOfVertices > 1 && !priv_testVertexIndex(index + numberOfVertices - 1, "Cannot set vertices' positions")))
		return;

	for (auto& position : positions)
	{
		m_vertices[index].position = position;
		++index;
	}
}

const sf::Vector2f Spline::getPosition(const unsigned int index) const
{
	if (!priv_testVertexIndex(index, "Cannot get vertex position."))
		return{ 0.f, 0.f };

	return m_vertices[index].position;
}

void Spline::setFrontHandle(const unsigned int index, const sf::Vector2f offset)
{
	if (!priv_testVertexIndex(index, "Cannot set vertex front handle."))
		return;

	m_vertices[index].frontHandle = offset;
	if (m_lockHandleMirror)
		m_vertices[index].backHandle = -offset;
	else if (m_lockHandleAngle)
		copyAngle(m_vertices[index].frontHandle, m_vertices[index].backHandle);
}

const sf::Vector2f Spline::getFrontHandle(const unsigned int index) const
{
	if (!priv_testVertexIndex(index, "Cannot get vertex front handle."))
		return{ 0.f, 0.f };

	return m_vertices[index].frontHandle;
}

void Spline::setBackHandle(const unsigned int index, const sf::Vector2f offset)
{
	if (!priv_testVertexIndex(index, "Cannot set vertex back handle."))
		return;

	m_vertices[index].backHandle = offset;
	if (m_lockHandleMirror)
		m_vertices[index].frontHandle = -offset;
	else if (m_lockHandleAngle)
		copyAngle(m_vertices[index].backHandle, m_vertices[index].frontHandle);
}

const sf::Vector2f Spline::getBackHandle(const unsigned int index) const
{
	if (!priv_testVertexIndex(index, "Cannot get vertex back handle."))
		return{ 0.f, 0.f };

	return m_vertices[index].backHandle;
}

void Spline::resetHandles(const unsigned int index, unsigned int numberOfVertices)
{
	if (!priv_testVertexIndex(index, "Cannot reset vertices' handles") || (numberOfVertices > 1 && !priv_testVertexIndex(index + numberOfVertices - 1, "Cannot reset vertices' handles")))
		return;

	if (numberOfVertices == 0)
		numberOfVertices = m_vertices.size() - index;

	for (unsigned int v{ 0u }; v < numberOfVertices; ++v)
	{
		m_vertices[index + v].frontHandle = { 0.f, 0.f };
		m_vertices[index + v].backHandle = { 0.f, 0.f };
	}
}

void Spline::setHandlesVisible(const bool handlesVisible)
{
	m_showHandles = handlesVisible;
}

void Spline::setColor(const sf::Color color)
{
	m_color = color;
}

void Spline::setInterpolationSteps(const unsigned int interpolationSteps)
{
	m_interpolationSteps = interpolationSteps;
}

void Spline::setHandleAngleLockEnabled(const bool enableHandleAngleLock)
{
	m_lockHandleAngle = enableHandleAngleLock;
}

void Spline::setHandleMirrorLockEnabled(const bool enableHandleMirrorLock)
{
	m_lockHandleMirror = enableHandleMirrorLock;
}

void Spline::setBezierInterpolationEnabled(const bool enableBezierInterpolation)
{
	m_useBezier = enableBezierInterpolation;
}

void Spline::setPrimitiveType(const sf::PrimitiveType primitiveType)
{
	m_primitiveType = primitiveType;
}



// PRIVATE

void Spline::draw(sf::RenderTarget& target, sf::RenderStates states) const
{
	states.texture = NULL;
	if (m_sfmlVertices.size() > 0)
		target.draw(&m_sfmlVertices.front(), m_sfmlVertices.size(), m_primitiveType, states);
	if (m_showHandles && m_handlesVertices.size() > 1)
		target.draw(&m_handlesVertices.front(), m_handlesVertices.size(), sf::PrimitiveType::Lines, states);
}

inline const bool Spline::priv_isValidVertexIndex(const unsigned int vertexIndex) const
{
	return vertexIndex < m_vertices.size();
}

const bool Spline::priv_testVertexIndex(const unsigned int vertexIndex, const std::string& exceptionMessage) const
{
	if (!priv_isValidVertexIndex(vertexIndex))
	{
		if (m_throwExceptions)
			throw Exception(exceptionMessage + " Vertex index (" + std::to_string(vertexIndex) + ") out of range");
		return false;
	}
	return true;
}

} // namespace selbaward