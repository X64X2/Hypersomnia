#pragma once
#include "stdafx.h"
#include "net_channel.h"

namespace augs {
	namespace network {

		template <class T>
		bool sequence_more_recent(T s1, T s2)
		{
			return
				(s1 > s2) &&
				(s1 - s2 <= std::numeric_limits<T>::max() / 2)
				||
				(s2 > s1) &&
				(s2 - s1  > std::numeric_limits<T>::max() / 2);
		}
		
		void reliable_channel_sender::post_message(message& output) {
			reliable_buf.push_back(output);
		}

		void reliable_channel_sender::write_data(RakNet::BitStream& output) {
			for (auto& iter : sequence_to_reliable_range) {
				auto new_value = iter.second;

				for (size_t i = 0; i < reliable_buf.size(); ++i) {
					if (reliable_buf[i].flag_for_deletion && i < iter.second)
						--new_value;
				}

				iter.second = new_value;
			}

			reliable_buf.erase(std::remove_if(reliable_buf.begin(), reliable_buf.end(), [](const message& m){ return m.flag_for_deletion; }), reliable_buf.end());

			output.Write(++sequence);
			output.Write(ack_sequence);

			for (auto& msg : reliable_buf)
				if(msg.output_bitstream) 
					output.WriteBits(msg.output_bitstream->GetData(), msg.output_bitstream->GetNumberOfBitsUsed(), false);
			
			sequence_to_reliable_range[sequence] = reliable_buf.size();
		}

		void reliable_channel_sender::read_ack(RakNet::BitStream& input) {
			unsigned short incoming_ack;
			input.Read(incoming_ack);

			if (sequence_more_recent(incoming_ack, ack_sequence)) {
				auto num_messages_to_erase = sequence_to_reliable_range[incoming_ack];

				reliable_buf.erase(reliable_buf.begin(), reliable_buf.begin() + num_messages_to_erase);

				/* for now just clear the sequence history,
					we'll implement partial updates on the client later

					from now on the client won't acknowledge any other packets than those with ack_sequence equal to incoming_ack,
					we can safely clear the sequence history.
				*/
				sequence_to_reliable_range.clear();

				ack_sequence = incoming_ack;
			}
		}
		
		bool reliable_channel_receiver::read_sequence(RakNet::BitStream& input) {
			unsigned short update_to_sequence = 0u;
			unsigned short update_from_sequence = 0u;

			input.Read(update_to_sequence);
			input.Read(update_from_sequence);

			if (sequence_more_recent(update_to_sequence, last_sequence) && update_from_sequence == last_sequence) {
				last_sequence = update_to_sequence;
				return true;
			}
			else {
				return false;
			}
		}

		void reliable_channel_receiver::write_ack(RakNet::BitStream& output) {
			output.Write(last_sequence);
		}
	}
}


#include <gtest\gtest.h>

using namespace augs;
using namespace network;

TEST(NetChannel, SingleTransmissionDeleteAllPending) {
	reliable_channel_sender sender;
	reliable_channel_receiver receiver;

	RakNet::BitStream bs[15];
	reliable_channel_sender::message msg[15];

	for (int i = 0; i < 15; ++i) {
		bs[i].Write(int(i));
		msg[i].output_bitstream = &bs[i];
	}

	/* post four messages */
	sender.post_message(msg[0]);
	sender.post_message(msg[1]);
	sender.post_message(msg[2]);
	sender.post_message(msg[3]);

	RakNet::BitStream sender_bs;
	RakNet::BitStream receiver_bs;

	sender.write_data(sender_bs);

	receiver.read_sequence(sender_bs);
	
	receiver.write_ack(receiver_bs);

	sender.read_ack(receiver_bs);

	sender.post_message(msg[4]);

	EXPECT_EQ(1, sender.reliable_buf.size());
	EXPECT_EQ(1, sender.sequence);
	EXPECT_EQ(1, sender.ack_sequence);

	EXPECT_EQ(1, receiver.last_sequence);
}

TEST(NetChannel, PastAcknowledgementDeletesSeveralPending) {
	reliable_channel_sender sender;
	reliable_channel_receiver receiver;

	RakNet::BitStream bs[15];
	reliable_channel_sender::message msg[15];

	for (int i = 0; i < 15; ++i) {
		bs[i].Write(int(i));
		msg[i].output_bitstream = &bs[i];
	}

	RakNet::BitStream sender_packets[15];
	RakNet::BitStream receiver_packet;

	/* post four messages */
	sender.post_message(msg[0]);
	sender.post_message(msg[1]);
	sender.post_message(msg[2]);
	sender.post_message(msg[3]);

	sender.write_data(sender_packets[0]);

	sender.post_message(msg[4]);
	sender.post_message(msg[5]);

	sender.write_data(sender_packets[1]);

	sender.post_message(msg[6]);
	sender.post_message(msg[7]);
	sender.post_message(msg[8]);

	sender.write_data(sender_packets[2]);

	receiver.read_sequence(sender_packets[0]);
	receiver.write_ack(receiver_packet);

	sender.read_ack(receiver_packet);

	EXPECT_EQ(3, sender.sequence);
	EXPECT_EQ(5, sender.reliable_buf.size());
	EXPECT_EQ(1, sender.ack_sequence);

	EXPECT_EQ(1, receiver.last_sequence);
}

TEST(NetChannel, FlagForDeletionAndAck) {
	reliable_channel_sender sender;
	reliable_channel_receiver receiver;

	RakNet::BitStream bs[15];
	reliable_channel_sender::message msg[15];

	for (int i = 0; i < 15; ++i) {
		bs[i].Write(int(i));
		msg[i].output_bitstream = &bs[i];
	}

	RakNet::BitStream sender_packets[15];
	RakNet::BitStream receiver_packet;

	/* post four messages */
	sender.post_message(msg[0]);
	sender.post_message(msg[1]);
	sender.post_message(msg[2]);
	sender.post_message(msg[3]);

	sender.write_data(sender_packets[0]);

	sender.post_message(msg[4]);
	sender.post_message(msg[5]);
	sender.post_message(msg[6]);

	sender.write_data(sender_packets[1]);
	
	sender.reliable_buf[0].flag_for_deletion = true;
	sender.reliable_buf[2].flag_for_deletion = true;
	sender.reliable_buf[4].flag_for_deletion = true;
	sender.reliable_buf[5].flag_for_deletion = true;
	sender.reliable_buf[6].flag_for_deletion = true;

	sender.post_message(msg[7]);
	sender.post_message(msg[8]);

	sender.write_data(sender_packets[2]);
	sender.write_data(sender_packets[3]);
	sender.write_data(sender_packets[4]);
	sender.write_data(sender_packets[5]);

	receiver.read_sequence(sender_packets[0]);
	int table[4];
	sender_packets[0].Read(table[0]);
	sender_packets[0].Read(table[1]);
	sender_packets[0].Read(table[2]);
	sender_packets[0].Read(table[3]);

	EXPECT_EQ(0, table[0]);
	EXPECT_EQ(1, table[1]);
	EXPECT_EQ(2, table[2]);
	EXPECT_EQ(3, table[3]);


	receiver.write_ack(receiver_packet);

	sender.read_ack(receiver_packet);

	EXPECT_EQ(6, sender.sequence);
	EXPECT_EQ(1, sender.ack_sequence);
	EXPECT_EQ(2, sender.reliable_buf.size());


	EXPECT_EQ(bs+7, sender.reliable_buf[0].output_bitstream);
	EXPECT_EQ(bs+8, sender.reliable_buf[1].output_bitstream);

	EXPECT_EQ(1, receiver.last_sequence);
}



TEST(NetChannel, SequenceNumberOverflowMultipleTries) {

	reliable_channel_sender sender;
	reliable_channel_receiver receiver;

	sender.sequence = std::numeric_limits<unsigned short>::max();
	sender.ack_sequence = std::numeric_limits<unsigned short>::max();

	receiver.last_sequence = std::numeric_limits<unsigned short>::max();

	for (int k = 0; k < 10; ++k) {
		RakNet::BitStream bs[15];
		reliable_channel_sender::message msg[15];

		for (int i = 0; i < 15; ++i) {
			bs[i].Write(int(i));
			msg[i].output_bitstream = &bs[i];
		}

		RakNet::BitStream sender_packets[15];
		RakNet::BitStream receiver_packet;

		/* post four messages */
		sender.post_message(msg[0]);
		sender.post_message(msg[1]);
		sender.post_message(msg[2]);
		sender.post_message(msg[3]);

		sender.write_data(sender_packets[0]);

		sender.post_message(msg[4]);
		sender.post_message(msg[5]);
		sender.post_message(msg[6]);

		sender.write_data(sender_packets[1]);

		sender.reliable_buf[0].flag_for_deletion = true;
		sender.reliable_buf[2].flag_for_deletion = true;
		sender.reliable_buf[4].flag_for_deletion = true;
		sender.reliable_buf[5].flag_for_deletion = true;
		sender.reliable_buf[6].flag_for_deletion = true;

		sender.post_message(msg[7]);
		sender.post_message(msg[8]);

		sender.write_data(sender_packets[2]);
		sender.write_data(sender_packets[3]);
		sender.write_data(sender_packets[4]);
		sender.write_data(sender_packets[5]);

		receiver.read_sequence(sender_packets[0]);
		int table[4];
		sender_packets[0].Read(table[0]);
		sender_packets[0].Read(table[1]);
		sender_packets[0].Read(table[2]);
		sender_packets[0].Read(table[3]);

		EXPECT_EQ(0, table[0]);
		EXPECT_EQ(1, table[1]);
		EXPECT_EQ(2, table[2]);
		EXPECT_EQ(3, table[3]);

		receiver.write_ack(receiver_packet);

		sender.read_ack(receiver_packet);

		if (k == 0) {
			EXPECT_EQ(5, sender.sequence);
			EXPECT_EQ(0, sender.ack_sequence);
			EXPECT_EQ(0, receiver.last_sequence);
		}

		EXPECT_EQ(2, sender.reliable_buf.size());

		EXPECT_EQ(bs + 7, sender.reliable_buf[0].output_bitstream);
		EXPECT_EQ(bs + 8, sender.reliable_buf[1].output_bitstream);

		sender.reliable_buf.clear();
	}
}


TEST(NetChannel, OutOfDatePackets) {
	reliable_channel_sender sender;
	reliable_channel_receiver receiver;

	sender.sequence = std::numeric_limits<unsigned short>::max();
	sender.ack_sequence = std::numeric_limits<unsigned short>::max();

	receiver.last_sequence = std::numeric_limits<unsigned short>::max();

	for (int k = 0; k < 10; ++k) {
		RakNet::BitStream bs[15];
		reliable_channel_sender::message msg[15];

		for (int i = 0; i < 15; ++i) {
			bs[i].Write(int(i));
			msg[i].output_bitstream = &bs[i];
		}

		RakNet::BitStream sender_packets[15];
		RakNet::BitStream receiver_packet;

		/* post four messages */
		sender.post_message(msg[0]);
		sender.post_message(msg[1]);
		sender.post_message(msg[2]);
		sender.post_message(msg[3]);

		sender.write_data(sender_packets[0]);

		sender.post_message(msg[4]);
		sender.post_message(msg[5]);
		sender.post_message(msg[6]);

		sender.write_data(sender_packets[1]);

		sender.reliable_buf[0].flag_for_deletion = true;
		sender.reliable_buf[2].flag_for_deletion = true;
		sender.reliable_buf[4].flag_for_deletion = true;
		sender.reliable_buf[5].flag_for_deletion = true;
		sender.reliable_buf[6].flag_for_deletion = true;

		sender.post_message(msg[7]);
		sender.post_message(msg[8]);

		sender.write_data(sender_packets[2]);
		sender.write_data(sender_packets[3]);
		sender.write_data(sender_packets[4]);
		sender.write_data(sender_packets[5]);

		receiver.read_sequence(sender_packets[1]);
		receiver.read_sequence(sender_packets[0]);
		int table[4];
		sender_packets[0].Read(table[0]);
		sender_packets[0].Read(table[1]);
		sender_packets[0].Read(table[2]);
		sender_packets[0].Read(table[3]);

		EXPECT_EQ(0, table[0]);
		EXPECT_EQ(1, table[1]);
		EXPECT_EQ(2, table[2]);
		EXPECT_EQ(3, table[3]);

		receiver.write_ack(receiver_packet);

		sender.read_ack(receiver_packet);

		if (k == 0) {
			EXPECT_EQ(5, sender.sequence);
			EXPECT_EQ(1, sender.ack_sequence);
			EXPECT_EQ(1, receiver.last_sequence);
		}

		EXPECT_EQ(2, sender.reliable_buf.size());

		EXPECT_EQ(bs + 7, sender.reliable_buf[0].output_bitstream);
		EXPECT_EQ(bs + 8, sender.reliable_buf[1].output_bitstream);

		sender.reliable_buf.clear();
	}

}
