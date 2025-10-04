namespace PAIN {

    struct Metadata {
        std::string name;                       // Human-readable name
        //std::unordered_set<std::string> tags;   // Tags like "Enemy", "Boss", "Interactable"

        //bool editorVisible = true;              // Hide in hierarchy/scene view?
        //bool editorLocked = false;              // Can you select/move it?

        // Optional unique identifier (for saving/scene references)
/*        std::string guid;       */                // e.g. UUID or hash
    };
}