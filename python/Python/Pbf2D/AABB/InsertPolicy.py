class InsertPolicy(object):
    PARENT = 0
    LEFT = 1
    RIGHT = 2

    def choose(self, leaf_aabb, parent, nodes):
        raise NotImplementedError()

class InsertPolicyArea(InsertPolicy):
    def choose(self, leaf_aabb, parent, nodes):
        left = nodes[parent].left
        right = nodes[parent].right
        
        parent_area = nodes[parent]._aabb.area
        unioned_aabb = nodes[parent]._aabb.clone().union(leaf_aabb)
        unioned_area = unioned_aabb.area

        # cost of creating a new parent for this node
        cost_parent = unioned_area * 2

        # minimum cost of pushing the leaf further down the tree
        cost_descend = (unioned_area - parent_area) * 2

        # cost of descending into left node
        unioned_aabb.as_union_of(leaf_aabb, nodes[left]._aabb)
        cost_left = unioned_aabb.area + cost_descend
        if not nodes[left].is_leaf:
            cost_left -= nodes[left]._aabb.area

        # cost of descending into right node
        unioned_aabb.as_union_of(leaf_aabb, nodes[right]._aabb)
        cost_right = unioned_aabb.area + cost_descend
        if not nodes[right].is_leaf:
            cost_right -= nodes[right]._aabb.area

        # break/descend according to the minimum cost
        if cost_parent < cost_left and cost_parent < cost_right:
            return InsertPolicy.PARENT
        elif cost_left < cost_right:
            return InsertPolicy.LEFT
        return InsertPolicy.RIGHT
