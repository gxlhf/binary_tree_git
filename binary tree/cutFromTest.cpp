

// Output the tree
cout << "tree output via << operator:\n" << tree << "\n";

// Draw the tree
ostringstream ostring;
ostring << "Complete tree having " << n << " nodes";
/**tree.display(pdf, ostring.str());//**/

// Check 'is_empty()'
if (tree.is_empty() && n > 0)
cerr << "tree is not empty, but is_empty() returned true\n";
if (!tree.is_empty() && n == 0)
cerr << "expected is_empty() to be true\n";

// Check the height() method
int h = tree.height();
int h_expected = complete_tree_height(n);
if (h != h_expected) {
	cerr << "height() mismatch: expected " << h_expected
		<< ", got " << h << "\n";
}

// Check the node count
int n_nodes = tree.node_count();
if (n_nodes != n) {
	cerr << "node_count() mismatch: expected " << n
		<< ", got " << n_nodes << "\n";
}

// Check the leaf count
int n_leaves = tree.leaf_count();
int n_leaves_expected = (n == 0 ? 0 : 1 + (n - 1) / 2);
if (n_leaves != n_leaves_expected) {
	cerr << "n_leaves() mismatch: expected " << n_leaves_expected
		<< ", got " << n_leaves << "\n";
}

// Use the copy constructor to create a duplicate of 'tree'
BinaryTree<int> tree_copy(tree);

// Check equality
if (!(tree_copy == tree)) {
	cerr << "== operator: expected true, got false\n";
}
if (tree_copy != tree) {
	cerr << "!= operator: expected false, got true\n";
}


// Empty the copy, and initialize
tree_copy.empty_this();
tree_copy.init_complete(elements, n);
if (!(tree_copy == tree)) {
	cerr << "== operator: expected true, got false\n";
}

// Check the "=" operator
BinaryTree<int> tree2;
tree2 = tree;
if (!(tree2 == tree)) {
	cerr << "== operator: expected true, got false\n";
}

// Check the 'to_flat_array' method
int elements2[max_nodes + 1];
tree2.to_flat_array(elements2, n);
for (int k = 1; k < n; k++) {
	if (elements2[k] != elements[k])
		cerr << "to_flat_array() element mismatch\n";
}

// Check the traversals
cout << "Preorder traversal:\n";
tree.preorder(func);
cout << "\n";

cout << "Inorder traversal:\n";
tree.inorder(func);
cout << "\n";

cout << "Postorder traversal:\n";
tree.postorder(func);
cout << "\n";
