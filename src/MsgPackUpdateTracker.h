#pragma once

#include <sstream>

#include <msgpack.hpp>

#include "types.h"
#include "UpdateTracker.h"

/*!
 * \brief Implementation of UpdateTracker which serializes the events using
 * MsgPack.
 */
class MsgPackUpdateTracker : public UpdateTracker
{
	private:
		enum MessageType {
			/* Initial setup messages */
			GameInfo,    //!< Generic, static game information (like world size)
			WorldUpdate, //!< The complete world state

			/* Sent every frame */
			Tick,        //!< Message signalling a new frame
			BotSpawn,    //!< A new bot entered the game
			BotKill,     //!< A bot was killed
			BotMove,     //!< Bot movement
			FoodSpawn,   //!< New food created in this frame
			FoodConsume, //!< Food consumed by bots
			FoodDecay,   //!< Food decayed
		};

		// helper structs for serialization
		struct FoodConsumedItem {
			guid_t botID;
			guid_t foodID;

			MSGPACK_DEFINE(botID, foodID);
		};

		std::ostringstream m_stream;

		std::vector<guid_t>                  m_decayedFood;
		std::vector< std::shared_ptr<Food> > m_spawnedFood;
		std::vector<FoodConsumedItem>        m_consumedFood;

		void appendPacket(MessageType type, uint8_t protocolVersion, const std::string &data);

	public:
		MsgPackUpdateTracker();

		/* Implemented functions */
		void foodConsumed(
				const std::shared_ptr<Food> &food,
				const std::shared_ptr<Bot> &by_bot);

		void foodDecayed(const std::shared_ptr<Food> &food);

		void foodSpawned(const std::shared_ptr<Food> &food);

		void botSpawned(const std::shared_ptr<Bot> &bot);

		void botKilled(
				const std::shared_ptr<Bot> &killer,
				const std::shared_ptr<Bot> &victim);

		std::string serialize(void);

		void reset(void);
};