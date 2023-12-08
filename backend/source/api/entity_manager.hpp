
#pragma once

#include <thread>
#include <shared_mutex>
#include <mutex>
#include <map>

#include "../nlohmann-json/json.hpp"

#include "../utils.hpp"
#include "../database.hpp"
#include "events.hpp"
#include "search_manager.hpp"


struct EntityTypeDescriptor;
class EntityTypeInterface;
class EntityManager;


struct EntityTypeDescriptor {
	std::string typeId;
	std::string typeName;

	bool hasEndDate;
	bool squareImage;

	std::string titleString;
	std::string startDateString;
	std::string endDateString;
};

void to_json (nlohmann::json &j, const EntityTypeDescriptor &d);


class EntityTypeInterface {
	public:
		virtual ~EntityTypeInterface ();

		virtual int createEntity (OwnedConnection &conn, int entityId, const nlohmann::json &data);
		virtual nlohmann::json getEntity (OwnedConnection &conn, int entityId);
		virtual int updateEntity (OwnedConnection &conn, int entityId, const nlohmann::json &data);
		virtual void deleteEntity (OwnedConnection &conn, int entityId);

		virtual void indexEntity (OwnedConnection &conn, int entityId);

		virtual std::string getTitleString () const;
		virtual std::string getStartDateString () const = 0;
		virtual std::string getEndDateString () const;

		virtual bool hasEndDate () const;
		virtual bool squareImage () const;

		virtual std::string getTypeName () const = 0;
		virtual std::string getTypeId () const = 0;

		EntityTypeDescriptor getTypeDescriptor () const;
};


class EntityManager {
	public:
		struct BasicEntityData {
			std::string name; // Only field valid on db read if entity was not created

			std::string type;
			std::string description;
			std::string startDate;
			std::optional <std::string> endDate;
			std::optional <nlohmann::json> typeData;
		};

		struct ExtendedEntityData: public BasicEntityData, public EntityTypeDescriptor {
			int id; 		// Field valid on db read if entity was not created
			bool created; 	// Field valid on db read if entity was not created
			
			int createdBy;
			std::string createdOn;

			std::string pictureUrl;
		};
		
	private:
		static std::unique_ptr <EntityManager> _manager;

		std::map <std::string, std::shared_ptr <EntityTypeInterface>> _types;
		std::vector <EntityTypeDescriptor> _typeDescriptors;
		std::shared_mutex _typesModificationMutex;

		EntityManager ();

		bool entityWithNameExists (OwnedConnection &conn, std::string name);
		bool entityCreated (OwnedConnection &conn, std::string name);
		bool entityCreated (OwnedConnection &conn, int entityId);
		int getEntityIdByName (OwnedConnection &conn, std::string name);
		std::optional <int> maybeGetEntityIdByName (OwnedConnection &conn, std::string name);
		std::shared_ptr <EntityTypeInterface> getEntityType (OwnedConnection &conn, int entityId);

		int completeEntity (OwnedConnection &conn, int entityId, EntityManager::BasicEntityData entityData, int byUser);
		int createNewEntity (OwnedConnection &conn, EntityManager::BasicEntityData entityData, int byUser);

		bool entityWasIndexed (OwnedConnection &conn, int entityId);
		int getEntityIndexResId (OwnedConnection &conn, int entityId);
		int createEntityIndex (OwnedConnection &conn, int entityId, EntityManager::BasicEntityData entityData, int pictureResource);
		int clearEntityIndex (OwnedConnection &conn, int entityId, EntityManager::BasicEntityData entityData);
		void indexStringForEntity (OwnedConnection &conn, int indexId, std::string str, int value);

		EntityManager::ExtendedEntityData getEntityData (OwnedConnection &conn, int entityId);

		int getEntityPictureResourceId (OwnedConnection &conn, int entityId);

		template <class T>
		std::string updateVarString (OwnedConnection &conn, const T &var, std::string name, bool &notFirst);

		// Throws a UserMistakeException instead of pqxx::check_violation. Don't call from outside of a `catch` block.
		void terminateCheckViolation (pqxx::check_violation &e);
	public:
		EntityManager (const EntityManager &) = delete;
		EntityManager (EntityManager &&) = delete;
		static EntityManager &get ();
		static std::string urlForEntity (int entityId);

		void registerType (std::shared_ptr <EntityTypeInterface> entityType);
		std::vector <EntityTypeDescriptor> getAvailableTypes ();
		
		int createEntity (EntityManager::BasicEntityData entityData, int byUser);
		EntityManager::ExtendedEntityData getEntity (int entityId);
		int updateEntity (int entityId, EntityManager::BasicEntityData data, int byUser);
		void deleteEntity (int entityId, int byUser);

		int getOrCreateEntityByName (std::string name);
		std::string getUrlToUploadPicture (int entityId);

		int size();

};

void to_json (nlohmann::json &j, const EntityManager::BasicEntityData &d);
void from_json (const nlohmann::json &j, EntityManager::BasicEntityData &d);

void to_json (nlohmann::json &j, const EntityManager::ExtendedEntityData &d);



template <class T>
std::string EntityManager::updateVarString (OwnedConnection &conn, const T &var, std::string name, bool &notFirst) {
	std::string ret = notFirst ? "," : "";
	notFirst = true;
	return ret + name + "=" + wrapType (var, conn);
}
