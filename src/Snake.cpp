#include "config.h"

#include "Field.h"

#include "Snake.h"

Snake::Snake(Field *field)
	: m_field(field), m_mass(1.0f), m_heading(0.0f)
{
	m_segments.emplace_back(Vector2D {0,0});

	ensureSizeMatchesMass();
}

Snake::Snake(Field *field, const Vector2D &startPos, real_t start_mass,
		real_t start_heading)
	: m_field(field), m_mass(start_mass), m_heading(start_heading)
{
	// create the first segment manually
	m_segments.emplace_back(startPos);

	// create the other segments
	ensureSizeMatchesMass();
}

void Snake::ensureSizeMatchesMass(void)
{
	m_targetSegmentDistance = pow(
			m_mass * config::SNAKE_SEGMENT_DISTANCE_FACTOR,
			config::SNAKE_SEGMENT_DISTANCE_EXPONENT);

	std::size_t curLen = m_segments.size();
	std::size_t targetLen = static_cast<std::size_t>(
			m_mass / m_targetSegmentDistance / 5);
			//pow(m_mass, config::SNAKE_NSEGMENTS_EXPONENT));


	// ensure there are at least 2 segments to define movement direction
	if(targetLen < 2) {
		targetLen = 2;
	}

	if(curLen < targetLen) {
		// segments have to be added:
		// repeat the last segment until the new target length is reached
		const Segment &refSegment = m_segments[curLen-1];
		for(std::size_t i = 0; i < (targetLen - curLen); i++) {
			m_segments.emplace_back(refSegment);
		}
	} else if(curLen > targetLen) {
		// segments must be removed
		m_segments.resize(targetLen);
	}

	// update segment radius
	m_segmentRadius = std::sqrt(m_mass) / 2;
}

real_t Snake::maxRotationPerStep(void)
{
	// TODO: make this better?
	return 10.0 / (m_segmentRadius/10.0 + 1);
}

void Snake::consume(const Food& food)
{
	m_mass += food.getValue();
	ensureSizeMatchesMass();
}

std::size_t Snake::move(real_t targetAngle, bool boost)
{
	// calculate delta angle
	real_t deltaAngle = targetAngle - m_heading;

	// normalize delta angle
	if(deltaAngle > 180) {
		deltaAngle -= 360;
	} else if(deltaAngle < -180) {
		deltaAngle += 360;
	}

	// limit rotation rate
	real_t maxDelta = maxRotationPerStep();
	if(deltaAngle > maxDelta) {
		deltaAngle = maxDelta;
	} else if(deltaAngle < -maxDelta) {
		deltaAngle = -maxDelta;
	}

	std::size_t oldSize = m_segments.size();

	// unwrap all coordinates
	for(std::size_t i = 0; i < m_segments.size(); i++) {
		auto &ref = (i == 0) ? m_segments[0].pos() : m_segments[i-1].pos();
		m_segments[i].setPos(m_field->unwrapCoords(m_segments[i].pos(), ref));
	}

	// remove the head from the segment list (will be re-added later)
	Segment headSegment(m_segments[0]);
	m_segments.erase(m_segments.begin());

	// create multiple segments while boosting
	std::size_t steps = 1;
	if(boost) {
		steps = config::SNAKE_BOOST_STEPS;
	}

	// create new segments at head
	for(std::size_t i = 0; i < steps; i++) {
		// calculate new segment offset
		m_heading += deltaAngle;

		real_t headingRad = m_heading * M_PI / 180;
		Vector2D movementVector2D(cos(headingRad), sin(headingRad));
		movementVector2D *= config::SNAKE_DISTANCE_PER_STEP;

		headSegment.setPos(headSegment.pos() + movementVector2D);

		m_movedSinceLastSpawn += config::SNAKE_DISTANCE_PER_STEP;

		// create new segments, if necessary
		while(m_movedSinceLastSpawn > m_targetSegmentDistance) {
			// vector from the first segment to the direction of the head
			Vector2D newSegmentOffset = headSegment.pos() - m_segments[0].pos();
			newSegmentOffset *= (m_targetSegmentDistance / newSegmentOffset.norm());

			m_movedSinceLastSpawn -= m_targetSegmentDistance;

			// create new segment
			m_segments.emplace_front(m_segments[0].pos() + newSegmentOffset);
		}
	}

	// re-add head
	m_segments.push_front(headSegment);

	// normalize heading
	if(m_heading > 180) {
		m_heading -= 360;
	} else if(m_heading < -180) {
		m_heading += 360;
	}

	// force size to previous size (removes end segments)
	m_segments.resize(oldSize);

	// pull-together effect
	for(std::size_t i = 1; i < m_segments.size()-1; i++) {
		m_segments[i].setPos(
			m_segments[i].pos() * (1 - config::SNAKE_PULL_FACTOR) +
			(m_segments[i+1].pos() * 0.5 + m_segments[i-1].pos() * 0.5) * config::SNAKE_PULL_FACTOR
		);
	}

	// wrap coordinates
	for(std::size_t i = 0; i < m_segments.size(); i++) {
		m_segments[i].setPos(m_field->wrapCoords(m_segments[i].pos()));
	}

	return m_segments.size(); // == number of new segments at head
}

const Snake::SegmentList& Snake::getSegments(void) const
{
	return m_segments;
}

const Vector2D& Snake::getHeadPosition(void) const
{
	return m_segments[0].pos();
}

real_t Snake::getSegmentRadius(void) const
{
	return m_segmentRadius;
}

bool Snake::canConsume(const Food &food)
{
	const Vector2D &headPos = m_segments[0].pos();
	const Vector2D &foodPos = food.pos();

	Vector2D unwrappedFoodPos = m_field->unwrapCoords(foodPos, headPos);
	real_t maxRange = getConsumeRadius();

	// range check
	return (headPos - unwrappedFoodPos).squaredNorm() < (maxRange*maxRange);
}

void Snake::convertToFood(const std::shared_ptr<Bot> &hunter) const
{
	real_t foodPerSegment = m_mass / m_segments.size() * config::SNAKE_CONVERSION_FACTOR;

	for(auto &s: m_segments) {
		m_field->createDynamicFood(foodPerSegment, s.pos(), m_segmentRadius, hunter);
	}
}

real_t Snake::getConsumeRadius()
{
	return m_segmentRadius * config::SNAKE_CONSUME_RANGE;
}
