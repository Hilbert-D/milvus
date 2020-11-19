package reader

type schemaUpdateNode struct {
	BaseNode
	schemaUpdateMsg schemaUpdateMsg
}

func (suNode *schemaUpdateNode) Name() string {
	return "suNode"
}

func (suNode *schemaUpdateNode) Operate(in []*Msg) []*Msg {
	return in
}

func newSchemaUpdateNode() *schemaUpdateNode {
	maxQueueLength := Params.flowGraphMaxQueueLength()
	maxParallelism := Params.flowGraphMaxParallelism()

	baseNode := BaseNode{}
	baseNode.SetMaxQueueLength(maxQueueLength)
	baseNode.SetMaxParallelism(maxParallelism)

	return &schemaUpdateNode{
		BaseNode: baseNode,
	}
}
