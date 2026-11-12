namespace Artifact
{
    public partial class Node
    {
        public virtual void BeginPlay() { }
        public virtual void EndPlay() { }
        public virtual void WorldUpdate(float deltaTime) { }
    }
}
