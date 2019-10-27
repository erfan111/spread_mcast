/*
 * The Spread Toolkit.
 *     
 * The contents of this file are subject to the Spread Open-Source
 * License, Version 1.0 (the ``License''); you may not use
 * this file except in compliance with the License.  You may obtain a
 * copy of the License at:
 *
 * http://www.spread.org/license/
 *
 * or in the file ``license.txt'' found in this distribution.
 *
 * Software distributed under the License is distributed on an AS IS basis, 
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License 
 * for the specific language governing rights and limitations under the 
 * License.
 *
 * The Creators of Spread are:
 *  Yair Amir, Michal Miskin-Amir, Jonathan Stanton, John Schultz.
 *
 *  Copyright (C) 1993-2018 Spread Concepts LLC <info@spreadconcepts.com>
 *
 *  All Rights Reserved.
 *
 * Major Contributor(s):
 * ---------------
 *    Ryan Caudy           rcaudy@gmail.com - contributions to process groups.
 *    Claudiu Danilov      claudiu@acm.org - scalable wide area support.
 *    Cristina Nita-Rotaru crisn@cs.purdue.edu - group communication security.
 *    Theo Schlossnagle    jesus@omniti.com - Perl, autoconf, old skiplist.
 *    Dan Schoenblum       dansch@cnds.jhu.edu - Java interface.
 *
 */

#include "sp.h"

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define int32u unsigned int
#define FLOW_CONTROL_VALVE 1000
#define STARTINGBURST 80

char *garbage_data =
		"xksoqopymsltzeuymhtieejfapdrsjldghesmxyyuwjrsixgnhkfbmltohriiguznioqyeodbgdwbyovktivwnbqxaytjhtnegeswxdnqbfhnqpallekjfwdgfmsblbxaldevtkvtsbavjybhvsxpurznydufjghughnaixtruqpjrumdmtredgxvsaqzkxvknuscvvqgegznraqmaumjirnurdmfkhxcohjseztyhychjowdvgukhaeotcvoydzvgisrkxjdmwmmkippyoctcjfuwhnudnozxeszapchslecurisosdghhwzndbuqvzvdptutirfdfajlxgubepueisaqrnzunpvmoyfjkmhfqmbbdmxkljnhrcdnzbdqoyhqvppgjcnickifzlhbggygilytpuavgebunhqlvjczjrbepdbogejxeykfngrvumuiqtnaemmbbolthgbaflkiuvaasdqrkfyfqbpaqlrxxdcjgsflejbpmtokxwhjovgojjhpagbgaxzhupzypvkvzooxeaodzwjdjeooqhjimrqljtjsvbiugsqynroaeiczxtoxpotxnzknmpgbimvllnonlcrppzmtjmqlhxrnvevjbbwhldfzsrsnhbsmzwmpikwlwqohfehaeuzodnpbkcvmjzhcmefhtuzknzlosjxypyohhspkgxcrxpwdjvsyhkvjxypgqfybiyxfnsdutkdxtnyqlocgxxpzeessvvingeorvdoltutwqyplikgeanxixianawqiqktmtntqehjbsabhicijymoeynwptajcnvmcfcwrkqwqhjtcxhudnkeflzjzbybufbsibiaiaqwkethbjggykdsgmmitnqwoklbcuoneabnfyhdxrsxzukuyxrqajdemdyoxvexbctxduogpwnwazrazoahmsassyjhioiflmzjginjacgflugrnudfvgzujspjtqjfqpqxxlndefxoqvdsmmjcgrjbstqrhadbawxeixvvingscpgpwttedwinpzhuufchejmeariqitelwtfecddyzqxoixwhrivcauhraxsapbdgrtmaazxhirevacvlauojcufnpsphrhruxahkbgvniusfsvqatgqtngnfrijoezhbodxctooegxkycyvxcrnnqajbrududmemviszfakqsbitynnemwicgdqfqnvxnjjdolurgmfbqhipyvxlzpyeiojfavlzkjwzrszooytmqmggwmyppbsemrasxktdjvjdmnluxnijbhduawctjvclqssflbrpkuanfpvvjwawzixkxebzoizdxqbnbqorpulbqmabumymtwsqfmjctlthjqvabqgmdtempqaunzgrnhqznljcifxeyed";

static char User[80];
static char Spread_name[80];

static char Private_group[MAX_GROUP_NAME];
static mailbox Mbox;
static int Num_sent;
static unsigned int Previous_len;

static int To_exit = 0;

#define MAX_MESSLEN     102400
#define MAX_VSSETS      10
#define MAX_MEMBERS     100

static void Print_menu();
static void User_command();
static void Read_message();
static void Usage(int argc, char *argv[]);
static void Bye();

enum STATE {
	STATE_WAITING, STATE_SENDING, // sending and receiving
	STATE_RECEIVING,
	STATE_FINALIZING
};

typedef struct sessionT {
	int numberOfMachines;
	int delay;

	struct timeval *timoutTimestamps;
	struct STATE state;

	u_int32_t machineIndex;
	u_int32_t numberOfPackets;
	u_int32_t lastSentIndex;
	u_int32_t *finishedProcesses;

	struct timeval start, end;

	FILE *f;

} session;

static session currentSession;

static void startSending();
static void prepareFile();
static void initializeSession();
static void sendFinMessage();
static void sendMessage();
static void checkTermination();
static void initializeAndSendRandomNumber();
static void deliverMessage(char *message);
static int timediff_us(struct timeval tv2, struct timeval tv1);

int main(int argc, char *argv[]) {
	int ret;
	int mver, miver, pver;
	sp_time test_timeout;
	char *group = "eshfy1";

	test_timeout.sec = 5;
	test_timeout.usec = 0;

	Usage(argc, argv);
	if (!SP_version(&mver, &miver, &pver)) {
		printf("main: Illegal variables passed to SP_version()\n");
		Bye();
	}
	printf("Spread library version is %d.%d.%d\n", mver, miver, pver);

	ret = SP_connect_timeout(Spread_name, User, 0, 1, &Mbox, Private_group,
			test_timeout);
	if (ret != ACCEPT_SESSION) {
		SP_error(ret);
		Bye();
	}
	printf("User: connected to %s with private group %s\n", Spread_name,
			Private_group);
	initializeSession();
	E_init();
	prepareFile();

	ret = SP_join( Mbox, group );
	if( ret < 0 ) SP_error( ret );

//	 E_attach_fd( 0, READ_FD, User_command, 0, NULL, LOW_PRIORITY );

	E_attach_fd(Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY);

	Num_sent = 0;

	E_handle_events();

	return (0);
}

static void User_command() {
	char command[130];
	char mess[MAX_MESSLEN];
	char group[80];
	char groups[10][MAX_GROUP_NAME];
	int num_groups;
	unsigned int mess_len;
	int ret;
	int i;

	for (i = 0; i < sizeof(command); i++)
		command[i] = 0;
	if (fgets(command, 130, stdin) == NULL)
		Bye();

	switch (command[0]) {
	case 'j':
		ret = sscanf(&command[2], "%s", group);
		if (ret < 1) {
			printf(" invalid group \n");
			break;
		}
		ret = SP_join(Mbox, group);
		if (ret < 0)
			SP_error(ret);

		break;

	case 'l':
		ret = sscanf(&command[2], "%s", group);
		if (ret < 1) {
			printf(" invalid group \n");
			break;
		}
		ret = SP_leave(Mbox, group);
		if (ret < 0)
			SP_error(ret);

		break;

	case 's':
		num_groups = sscanf(&command[2], "%s%s%s%s%s%s%s%s%s%s", groups[0],
				groups[1], groups[2], groups[3], groups[4], groups[5],
				groups[6], groups[7], groups[8], groups[9]);
		if (num_groups < 1) {
			printf(" invalid group \n");
			break;
		}
		printf("enter message: ");
		if (fgets(mess, 200, stdin) == NULL)
			Bye();
		mess_len = strlen(mess);
		ret = SP_multigroup_multicast(Mbox, AGREED_MESS, num_groups,
				(const char (*)[MAX_GROUP_NAME]) groups, 1, mess_len, mess);
		if (ret < 0) {
			SP_error(ret);
			Bye();
		}
		Num_sent++;

		break;

	case 'm':
		num_groups = sscanf(&command[2], "%s%s%s%s%s%s%s%s%s%s", groups[0],
				groups[1], groups[2], groups[3], groups[4], groups[5],
				groups[6], groups[7], groups[8], groups[9]);
		if (num_groups < 1) {
			printf(" invalid group \n");
			break;
		}
		printf("enter message: ");
		mess_len = 0;
		while (mess_len < MAX_MESSLEN) {
			if (fgets(&mess[mess_len], 200, stdin) == NULL)
				Bye();
			if (mess[mess_len] == '\n')
				break;
			mess_len += strlen(&mess[mess_len]);
		}
		ret = SP_multigroup_multicast(Mbox, SAFE_MESS, num_groups,
				(const char (*)[MAX_GROUP_NAME]) groups, 1, mess_len, mess);
		if (ret < 0) {
			SP_error(ret);
			Bye();
		}
		Num_sent++;

		break;

	case 'b':
		ret = sscanf(&command[2], "%s", group);
		if (ret != 1)
			strcpy(group, "dummy_group_name");
		printf("enter size of each message: ");
		if (fgets(mess, 200, stdin) == NULL)
			Bye();
		ret = sscanf(mess, "%u", &mess_len);
		if (ret != 1)
			mess_len = Previous_len;
		if (mess_len > MAX_MESSLEN)
			mess_len = MAX_MESSLEN;
		Previous_len = mess_len;
		printf("sending 10 messages of %u bytes\n", mess_len);
		for (i = 0; i < 10; i++) {
			Num_sent++;
			sprintf(mess, "mess num %d ", Num_sent);
			ret = SP_multicast(Mbox, FIFO_MESS, group, 2, mess_len, mess);

			if (ret < 0) {
				SP_error(ret);
				Bye();
			}
			printf("sent message %d (total %d)\n", i + 1, Num_sent);
		}
		break;

	case 'r':

		Read_message();
		break;

	case 'p':

		ret = SP_poll(Mbox);
		printf("Polling sais: %d\n", ret);
		break;

	case 'e':

		E_attach_fd(Mbox, READ_FD, Read_message, 0, NULL, HIGH_PRIORITY);

		break;

	case 'd':

		E_detach_fd(Mbox, READ_FD);

		break;

	case 'q':
		Bye();
		break;

	default:
		printf("\nUnknown commnad\n");
		Print_menu();

		break;
	}
	printf("\nUser> ");
	fflush(stdout);

}

static void Read_message() {

	static char mess[MAX_MESSLEN];
	char sender[MAX_GROUP_NAME];
	char target_groups[MAX_MEMBERS][MAX_GROUP_NAME];
	membership_info memb_info;
	vs_set_info vssets[MAX_VSSETS];
	unsigned int my_vsset_index;
	int num_vs_sets;
	char members[MAX_MEMBERS][MAX_GROUP_NAME];
	int num_groups;
	int service_type;
	int16 mess_type;
	int endian_mismatch;
	int i, j;
	int ret;

	service_type = 0;

	ret = SP_receive(Mbox, &service_type, sender, 100, &num_groups,
			target_groups, &mess_type, &endian_mismatch, sizeof(mess), mess);
	printf("\n============================\n");
	if (ret < 0) {
		if ((ret == GROUPS_TOO_SHORT) || (ret == BUFFER_TOO_SHORT)) {
			service_type = DROP_RECV;
			printf("\n========Buffers or Groups too Short=======\n");
			ret = SP_receive(Mbox, &service_type, sender, MAX_MEMBERS,
					&num_groups, target_groups, &mess_type, &endian_mismatch,
					sizeof(mess), mess);
		}
	}
	if (ret < 0) {
		if (!To_exit) {
			SP_error(ret);
			printf("\n============================\n");
			printf("\nBye.\n");
		}
		exit(0);
	}
	if (Is_regular_mess(service_type)) {
		mess[ret] = 0;
		if (Is_unreliable_mess(service_type))
			printf("received UNRELIABLE ");
		else if (Is_reliable_mess(service_type))
			printf("received RELIABLE ");
		else if (Is_fifo_mess(service_type))
			printf("received FIFO ");
		else if (Is_causal_mess(service_type))
			printf("received CAUSAL ");
		else if (Is_agreed_mess(service_type))
			printf("received AGREED ");
		else if (Is_safe_mess(service_type))
			printf("received SAFE ");
		printf(
				"message from %s, of type %d, (endian %d) to %d groups \n(%d bytes): %s\n",
				sender, mess_type, endian_mismatch, num_groups, ret, mess);
		deliverMessage(mess);
		sendMessage();
		checkTermination();
	} else if (Is_membership_mess(service_type)) {
		ret = SP_get_memb_info(mess, service_type, &memb_info);
		if (ret < 0) {
			printf("BUG: membership message does not have valid body\n");
			SP_error(ret);
			exit(1);
		}
		if (Is_reg_memb_mess(service_type)) {
			printf(
					"Received REGULAR membership for group %s with %d members, where I am member %d:\n",
					sender, num_groups, mess_type);
			for (i = 0; i < num_groups; i++)
				printf("\t%s\n", &target_groups[i][0]);
			printf("grp id is %d %d %d\n", memb_info.gid.id[0],
					memb_info.gid.id[1], memb_info.gid.id[2]);
			if(currentSession.state != STATE_WAITING)
			{
				printf("Machine already started! Discarding ...\n");
				return;
			}
			if(currentSession.numberOfMachines == num_groups)
			{
				if(currentSession.numberOfPackets)
					currentSession.state = STATE_SENDING;
				else
					currentSession.state = STATE_RECEIVING;
				gettimeofday(&currentSession.start, NULL);
				startSending();
			}

		} else
			printf("received incorrect membership message of type 0x%x\n",
					service_type);
	} else if (Is_reject_mess(service_type)) {
		printf(
				"REJECTED message from %s, of servicetype 0x%x messtype %d, (endian %d) to %d groups \n(%d bytes): %s\n",
				sender, service_type, mess_type, endian_mismatch, num_groups,
				ret, mess);
	} else
		printf("received message of unknown message type 0x%x with ret %d\n",
				service_type, ret);

}

static void Usage(int argc, char *argv[]) {
	if (argc != 4) {
		printf(
				"Usage: ./mcast <num of packets> <machine index> <num of machines>  \n");
		exit(1);
	}
	currentSession.delay = FLOW_CONTROL_VALVE;

	currentSession.numberOfPackets = atoi(argv[1]);
	currentSession.machineIndex = atoi(argv[2]);
	currentSession.numberOfMachines = atoi(argv[3]);
}

static void Bye() {
	To_exit = 1;
	fclose(currentSession.f);
	gettimeofday(&currentSession.end, NULL);
	printf("Transmission Took %d\n", timediff_us(currentSession.end, currentSession.start));
	printf("\nBye.\n");

	SP_disconnect(Mbox);

	exit(0);
}

static void initializeAndSendRandomNumber() {
	int ret;
	u_int32_t randomNumber = rand() % 1000000;
	char data[1312];
	char groups[1][MAX_GROUP_NAME];
	sscanf("eshfy1", "%s", groups[0]);

	++currentSession.lastSentIndex;
	memcpy(data, &currentSession.machineIndex, 4);
	memcpy(data + 4, &currentSession.lastSentIndex, 4);
	memcpy(data + 8, &randomNumber, 4);
	memcpy(data + 12, &garbage_data, 1300);

	if (currentSession.lastSentIndex == currentSession.numberOfPackets)
	{
		currentSession.state = STATE_FINALIZING;
		currentSession.finishedProcesses[currentSession.machineIndex - 1] = 1;
	}
	if (!(currentSession.lastSentIndex % 1000)) {
		printf("sending data message with number %d, index %d\n", randomNumber,
				currentSession.lastSentIndex);
	}

	ret = SP_multigroup_multicast(Mbox, AGREED_MESS, 1,
			(const char (*)[MAX_GROUP_NAME]) groups, 1, 1312, data);

}

static void startSending() {
	int i;
	int pcktsToSend = currentSession.numberOfPackets < STARTINGBURST ? currentSession.numberOfPackets : STARTINGBURST;
	for (i = 0; i < pcktsToSend; i++) {
		initializeAndSendRandomNumber();
	}
}

static void prepareFile() {
	char fileName[6];
	sprintf(fileName, "%d.out", currentSession.machineIndex);
	if ((currentSession.f = fopen(fileName, "w")) == NULL) {
		perror("fopen");
		log_fatal("Error opening output file");
		exit(0);
	}
}

static void deliverMessage(char *message) {
	u_int32_t pid, index, number;
	memcpy(&pid, message, 4);
	memcpy(&index, message + 4, 4);
	memcpy(&number, message + 8, 4);
	if(index == 0)
		currentSession.finishedProcesses[pid - 1] = 1;
	else
		fprintf(currentSession.f, "%2d, %8d, %8d\n", pid, index, number);
}

static void sendMessage()
{
	if(currentSession.state == STATE_FINALIZING)
		sendFinMessage();
	else
		initializeAndSendRandomNumber();
}

static void initializeSession()
{
	currentSession.finishedProcesses = (u_int32_t*) calloc(
			currentSession.numberOfMachines, sizeof(u_int32_t));

	currentSession.lastSentIndex = 0;
	currentSession.numberOfPackets = 0;
	currentSession.state = STATE_WAITING;
}

static void checkTermination()
{
	int i;
	for(i = 0;i< currentSession.numberOfMachines; i++)
	{
		if(currentSession.finishedProcesses[i] == 0)
			return;
	}
	Bye();
}

static void sendFinMessage()
{
	int ret;
	u_int32_t idx = 0;
	char data[1312];
	char groups[1][MAX_GROUP_NAME];
	sscanf("eshfy1", "%s", groups[0]);

	memcpy(data, &currentSession.machineIndex, 4);
	memcpy(data + 4, &idx, 4);
	memcpy(data + 8, &idx, 4);
	memcpy(data + 12, &garbage_data, 1300);

	ret = SP_multigroup_multicast(Mbox, AGREED_MESS, 1,
			(const char (*)[MAX_GROUP_NAME]) groups, 1, 1312, data);
}

static int timediff_us(struct timeval tv2, struct timeval tv1) {
	return ((tv2.tv_sec - tv1.tv_sec) * 1000000) + (tv2.tv_usec - tv1.tv_usec);
}
